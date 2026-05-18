"""Command-line interface for the embedded CMake generator.

Usage::

    python -m embedded_cmake [options]
    python embedded_cmake/cli.py [options]
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from typing import Any, Dict, List, Optional

from . import __version__
from .chip_db import ChipDB
from .toolchain import ToolchainRegistry
from .config import load_config
from .scanner import (analyze_feature_flags, detect_config_h_conflicts,
                       scan_headers, scan_sources,
                       suggest_board_features)
from .generator import generate_all
from .utils import (detect_monolithic_includes, is_sandboxed,
                     log_error, log_info, log_warn, set_log_stderr)


def main(argv: Optional[List[str]] = None) -> int:
    """Main entry point. Returns exit code."""
    parser = _build_parser()
    args = parser.parse_args(argv)

    project_dir = os.path.abspath(args.project_dir)
    if not os.path.isdir(project_dir):
        log_error(f"Project directory not found: {project_dir}")
        return 1

    if args.json:
        set_log_stderr(True)

    log_info(f"Embedded CMake Generator v{__version__}")
    log_info(f"Project directory: {project_dir}")

    # -- Load data registries ------------------------------------------------
    chip_db = ChipDB()
    if args.chip_db_path:
        chip_db.add_search_path(args.chip_db_path)
    chip_db.load_builtin()

    tc_registry = ToolchainRegistry()
    if args.toolchain_data_path:
        tc_registry.add_search_path(args.toolchain_data_path)
    tc_registry.load_builtin()

    # -- Detect / load config ------------------------------------------------
    log_info("Detecting project configuration...")
    config = load_config(project_dir, chip_db, tc_registry)

    # Apply CLI overrides
    _apply_cli_overrides(config, chip_db, args)

    # Print detected config summary
    _print_config_summary(config)

    # -- Scan sources --------------------------------------------------------
    log_info("Scanning source files...")
    scan = scan_sources(project_dir, config.scan, chip_family=config.chip_family)
    hdr_dirs = scan_headers(project_dir, config.scan, chip_family=config.chip_family)

    # Merge user-specified lib files from config (embedded-cmake.json) into scan-resolved libs
    # For Keil projects the uvprojx lib list is authoritative (applied below)
    if not config.is_keil_project:
        for lib in config.lib_files:
            if lib not in scan.lib_files:
                scan.lib_files.append(lib)

    # Keil project: use Keil's source list as authoritative
    if config.is_keil_project and config.keil_source_groups:
        keil_c_sources: List[str] = []
        keil_asm_sources: List[str] = []
        for g in config.keil_source_groups:
            for sf in g.source_files:
                abs_path = os.path.normpath(os.path.join(project_dir, sf))
                if os.path.isfile(abs_path):
                    ext = os.path.splitext(sf)[1].lower()
                    if ext in (".s", ".asm"):
                        keil_asm_sources.append(abs_path)
                    elif ext == ".c":
                        keil_c_sources.append(abs_path)

        if keil_c_sources:
            scan.c_sources = sorted(keil_c_sources)
        if keil_asm_sources:
            scan.asm_sources = sorted(keil_asm_sources)

        leaf_files = detect_monolithic_includes(scan.c_sources)
        if leaf_files.leaf_files:
            scan.c_sources = [s for s in scan.c_sources if s not in leaf_files.leaf_files]

        # Also use Keil's lib list as authoritative (mirrors C/ASM pattern above)
        keil_libs: List[str] = []
        for lib in config.lib_files:
            abs_path = os.path.normpath(os.path.join(project_dir, lib))
            if os.path.isfile(abs_path):
                keil_libs.append(abs_path)
        if keil_libs:
            scan.lib_files = sorted(keil_libs)

    hdr_dirs = detect_config_h_conflicts(hdr_dirs, project_dir)

    if config.is_keil_project and config.keil_include_paths:
        keil_hdr_dirs = [kip for kip in config.keil_include_paths if os.path.isdir(kip)]
        for d in hdr_dirs:
            if d not in keil_hdr_dirs:
                keil_hdr_dirs.append(d)
        hdr_dirs = keil_hdr_dirs

    # CubeMX extras: merge HAL/CMSIS includes and source files
    if config.cubemx_extra_includes:
        for d in config.cubemx_extra_includes:
            if d not in hdr_dirs:
                hdr_dirs.append(d)
    if config.cubemx_extra_sources:
        for s in config.cubemx_extra_sources:
            if s not in scan.c_sources:
                scan.c_sources.append(s)

    log_info(f"  C sources: {len(scan.c_sources)}")
    log_info(f"  C++ sources: {len(scan.cpp_sources)}")
    log_info(f"  ASM sources: {len(scan.asm_sources)}")
    log_info(f"  Header dirs: {len(hdr_dirs)}")
    if scan.lib_files:
        log_info(f"  Lib files: {len(scan.lib_files)}")

    if args.check_features:
        log_info("Checking feature flags...")
        _run_feature_check(config, hdr_dirs)

    if args.suggest_features:
        log_info("Analyzing board-specific feature flags...")
        _run_suggest_features(config, hdr_dirs)

    if args.dry_run:
        if args.json:
            json_result = _build_json_output(config, scan, hdr_dirs, [], project_dir)
            print(json.dumps(json_result, indent=2))
        else:
            log_info("Dry run — skipping file generation.")
            _print_dry_run_details(config, scan, hdr_dirs)
        return 0

    # -- Generate -----------------------------------------------------------
    if args.json:
        log_info("Generating CMake files...")
    generated = generate_all(project_dir, config.cmake_dir, config, scan, hdr_dirs,
                             toolchain_registry=tc_registry)

    if args.json:
        json_result = _build_json_output(config, scan, hdr_dirs, generated, project_dir)
        print(json.dumps(json_result, indent=2))
        return 0

    if args.json:
        json_result = _build_json_output(config, scan, hdr_dirs, generated, project_dir)
        print(json.dumps(json_result, indent=2))
        return 0

    log_info(f"Generated {len(generated)} files:")
    for f in generated:
        print(f"  - {os.path.relpath(f, project_dir)}")

    print()

    if args.build or args.verify:
        if is_sandboxed() and "armcc" in config.toolchains:
            log_warn("ARMCC build requires native Windows host execution.")
            print("  sandbox_execution_refused=true")
            print("\n  Generate CMake files on the host and run:")
            cmd = (
                f"cd {os.path.join(config.project_dir, args.build_dir)} &&"
                f" cmake .. -G \"MinGW Makefiles\""
                f" -DCMAKE_TOOLCHAIN_FILE=../{config.cmake_dir}/armcc-toolchain.cmake"
                f" && make -j4"
            )
            print(f"  {cmd}")
            return 0

        ret = _run_build(config, project_dir, args.build_dir)
        if ret != 0:
            return ret
        if args.verify:
            _verify_artifacts(config, args.build_dir)
        return 0

    log_info("Next steps:")
    _print_build_instructions(config)

    return 0


# ---------------------------------------------------------------------------
# CLI helpers
# ---------------------------------------------------------------------------

def _run_build(config, project_dir: str, build_dir: str) -> int:
    """Run cmake configure + cmake --build for each toolchain. Returns 0 on success."""
    abs_build_dir = os.path.join(project_dir, build_dir)
    os.makedirs(abs_build_dir, exist_ok=True)

    for tc in config.toolchains:
        log_info(f"Configuring {tc} build...")
        toolchain_file = f"../{config.cmake_dir}/{tc}-toolchain.cmake"
        result = subprocess.run(
            ["cmake", "..",
             "-G", "MinGW Makefiles",
             f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}"],
            cwd=abs_build_dir,
            capture_output=True, text=True,
        )
        if result.returncode != 0:
            log_error(f"CMake configure failed for {tc}:")
            log_error(result.stderr.strip())
            return result.returncode

        log_info(f"Building with {tc}...")
        result = subprocess.run(
            ["cmake", "--build", ".", "--", "-j4"],
            cwd=abs_build_dir,
            capture_output=True, text=True,
        )
        if result.returncode != 0:
            log_error(f"Build failed for {tc}:")
            log_error(result.stderr.strip())
            return result.returncode

        log_info(f"{tc} build successful.")

    return 0


def _verify_artifacts(config, build_dir: str) -> None:
    """Check that expected build output artifacts exist."""
    ext_map = {"armcc": "axf", "gcc": "elf"}
    for tc in config.toolchains:
        ext = ext_map.get(tc, "elf")
        artifact = os.path.join(build_dir, f"{config.project_name}.{ext}")
        if os.path.isfile(artifact):
            size = os.path.getsize(artifact)
            log_info(f"  {tc} artifact: {artifact} ({size} bytes)")
        else:
            log_warn(f"  {tc} artifact not found: {artifact}")


def _build_json_output(config, scan, hdr_dirs, generated, project_dir) -> Dict[str, Any]:
    """Build machine-readable JSON result dictionary."""
    return {
        "version": __version__,
        "project_type": (
            "keil" if config.is_keil_project
            else "cubemx" if config.is_cubemx_project
            else "generic"
        ),
        "detected_file": (
            config.keil_project_file or config.ioc_file or None
        ),
        "chip_family": config.chip_family,
        "chip_model": config.chip_model,
        "cpu_core": config.chip_info.cpu.core if config.chip_info else None,
        "toolchains": config.toolchains,
        "rtos": config.rtos.type if config.rtos else "none",
        "monolithic": config.monolithic is not None,
        "sources": {
            "c": len(scan.c_sources),
            "cpp": len(scan.cpp_sources),
            "asm": len(scan.asm_sources),
            "lib": len(scan.lib_files),
        },
        "header_dirs": len(hdr_dirs),
        "defines": config.defines,
        "generated_files": [
            os.path.relpath(f, project_dir) for f in generated
        ],
        "output_dir": config.cmake_dir,
        "build_commands": {
            tc: {
                "configure": (
                    f"cmake .. -G \"MinGW Makefiles\""
                    f" -DCMAKE_TOOLCHAIN_FILE=../{config.cmake_dir}/{tc}-toolchain.cmake"
                ),
                "build": "make -j4",
            }
            for tc in config.toolchains
        },
    }

def _apply_cli_overrides(config, chip_db, args) -> None:
    """Apply command-line argument overrides to the config."""
    if args.chip_family:
        config.chip_family = args.chip_family
        config.chip_info = chip_db.find_family(args.chip_family)
    if args.chip_model:
        config.chip_model = args.chip_model
        info = chip_db.find_family_for_model(args.chip_model)
        if info:
            config.chip_info = info
            config.chip_family = info.family
    if args.toolchain:
        config.toolchains = [args.toolchain]
    if args.armcc_dir:
        config.armcc_compiler_dir = args.armcc_dir
    if args.output_dir != "cmake":
        config.cmake_dir = args.output_dir
    if args.project_name:
        config.project_name = args.project_name
    if args.board:
        config.board_name = args.board
    if args.cpu:
        config.cpu_name = args.cpu
    if args.keil_project:
        _load_explicit_keil_project(config, args.keil_project)


def _load_explicit_keil_project(config, keil_project_path: str) -> None:
    """Load a Keil project specified via ``--keil-project``."""
    from .parsers import uvprojx_parser
    try:
        data = uvprojx_parser.parse_uvprojx(keil_project_path, config.project_dir)
        config.is_keil_project = True
        config.keil_project_file = keil_project_path
        for d in uvprojx_parser.extract_defines(data):
            if d not in config.defines:
                config.defines.append(d)
        config.keil_include_paths = uvprojx_parser.extract_include_paths(data)
        config.lib_files = uvprojx_parser.extract_lib_files(data)
        scatter = uvprojx_parser.extract_scatter_file(data)
        if scatter:
            config.armcc_scatter_file = scatter
        config.monolithic = data.get("monolithic")
        config.uses_microlib = data.get("uses_microlib", True)
        config.armcc_gnu_mode = data.get("gnu_mode", False)
        config.compiler_standard = data.get("compiler_standard", "C99")
        config.armcc_optimization = data.get("optimization", "")
        config.armcc_misc_cflags = data.get("misc_compiler_flags", "")
        config.armcc_misc_ldflags = data.get("misc_linker_flags", "")
        config.armcc_asm_defines = data.get("asm_defines", [])
        rtos_type = uvprojx_parser.detect_rtos_name(data)
        if rtos_type:
            config.rtos.type = rtos_type
        output_name = data.get("output_name", "")
        if output_name:
            config.project_name = output_name
    except Exception as e:
        log_warn(f"Failed to parse Keil project: {e}")


def _print_config_summary(config) -> None:
    """Print a summary of the detected configuration."""
    log_info(f"  Project: {config.project_name}")
    if config.chip_info:
        log_info(f"  Chip: {config.chip_info.family} / {config.chip_model} "
                 f"({config.chip_info.cpu.core})")
    log_info(f"  Toolchains: {', '.join(config.toolchains)}")
    if config.rtos.type != "none":
        log_info(f"  RTOS: {config.rtos.type}")
    if config.is_cubemx_project:
        log_info(f"  CubeMX: {os.path.basename(config.ioc_file)}")
    if config.is_keil_project and config.keil_project_file:
        log_info(f"  Keil project: {os.path.basename(config.keil_project_file)}")
    if config.lib_files:
        log_info(f"  Pre-compiled libs: {len(config.lib_files)}")
    if config.monolithic and config.monolithic.leaf_files:
        log_info(f"  Monolithic includes: {len(config.monolithic.leaf_files)} leaf files")


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Universal Embedded CMake Build System Generator",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python -m embedded_cmake                           # Auto-detect and generate
  python -m embedded_cmake -d /path/to/project       # Specify project directory
  python -m embedded_cmake --chip-family STM32F4     # Override chip family
  python -m embedded_cmake --toolchain gcc           # Force GCC toolchain
  python -m embedded_cmake --dry-run                 # Preview without generating
  python -m embedded_cmake --init                    # Generate config template
        """,
    )

    parser.add_argument("-d", "--project-dir", default=".",
                        help="Project root directory (default: current directory)")
    parser.add_argument("-n", "--project-name", default="",
                        help="Project name (default: auto-detect)")
    parser.add_argument("-o", "--output-dir", default="cmake",
                        help="CMake files output directory (default: cmake/)")
    parser.add_argument("-t", "--toolchain", default="",
                        choices=["gcc", "armcc", "iar", "clang"],
                        help="Force specific toolchain (default: auto-detect)")
    parser.add_argument("--chip-family", default="",
                        help="Override chip family (e.g. STM32F1, STM32F4)")
    parser.add_argument("--chip-model", default="",
                        help="Override chip model (e.g. STM32F103C8Tx)")
    parser.add_argument("--armcc-dir", default="",
                        help="ARMCC compiler binary directory")
    parser.add_argument("--chip-db-path", default="",
                        help="Additional path for custom chip database JSON files")
    parser.add_argument("--toolchain-data-path", default="",
                        help="Additional path for custom toolchain JSON files")
    parser.add_argument("--dry-run", action="store_true",
                        help="Detect and print configuration without generating files")
    parser.add_argument("--board", default="",
                        help="Board name for board-specific config")
    parser.add_argument("--cpu", default="",
                        help="CPU/SoC override (e.g. APM32E103RE)")
    parser.add_argument("--keil-project", default="",
                        help="Path to .uvprojx file (overrides auto-discovery)")
    parser.add_argument("--init", action="store_true",
                        help="Generate an embedded-cmake.json template file")
    parser.add_argument("--version", action="version",
                        version=f"embedded-cmake-generator v{__version__}")
    parser.add_argument("--check-features", action="store_true",
                        help="Scan config.h for disabled/commented-out feature flags")
    parser.add_argument("--suggest-features", action="store_true",
                        help="Suggest feature flags needed for the active board")
    parser.add_argument("--json", action="store_true",
                        help="Output machine-readable JSON instead of human-readable logs")
    parser.add_argument("--build", action="store_true",
                        help="Generate CMake files then run cmake --build")
    parser.add_argument("--build-dir", default="build",
                        help="Build directory (default: build/)")
    parser.add_argument("--verify", action="store_true",
                        help="Generate, build, and check output artifacts exist")

    return parser


def _print_build_instructions(config) -> None:
    """Print build instructions for detected toolchains."""
    for tc_id in config.toolchains:
        tc_name = tc_id.upper()
        print(f"  [{tc_name}]")
        print(f"    mkdir -p build && cd build")
        print(f"    cmake .. -G \"MinGW Makefiles\" "
              f"-DCMAKE_TOOLCHAIN_FILE=../{config.cmake_dir}/{tc_id}-toolchain.cmake")
        if tc_id == "armcc":
            print(f"    make -j4")
            if config.armcc_compiler_dir:
                print(f"    (ARMCC compiler dir: {config.armcc_compiler_dir})")
        else:
            print(f"    make -j4")
    print()
    if "armcc" in config.toolchains:
        log_info("Note: ARMCC toolchain requires configuring the compiler path")
        log_info(f"  in {config.cmake_dir}/armcc-toolchain.cmake if not auto-detected")


def _run_feature_check(config, hdr_dirs) -> None:
    """Run feature flag diagnostics on the primary config.h."""
    # Find the primary config.h directory (first in the priority list)
    if not hdr_dirs:
        return
    config_h_dir = hdr_dirs[0]  # after conflict detection, primary is first
    flags = analyze_feature_flags(config.project_dir, config_h_dir,
                                   config.defines)
    if not flags:
        log_info("  No disabled feature flags detected.")
        return

    log_warn(f"  Found {len(flags)} potentially disabled feature flag(s):")
    for f in flags:
        log_warn(f"    {f['flag']}  — {f['detail']}")
    print()
    log_info("Tip: Define them via `defines` in embedded-cmake.json")
    log_info("     or via the `-D` compiler flag.")


def _run_suggest_features(config, hdr_dirs) -> None:
    """Analyze config.h and suggest feature flags for the active board."""
    if not hdr_dirs:
        log_info("  No header directories — skipping feature suggestion.")
        return
    config_h_dir = hdr_dirs[0]

    result = suggest_board_features(config.project_dir, config_h_dir,
                                     config.defines)
    if not result:
        log_info("  No config.h found — skipping feature suggestion.")
        return
    if "error" in result:
        log_warn(f"  {result['error']}")
        return

    print()
    log_info(f"Board: {result['board_name']} = {result['board_value']}")
    log_info(f"Board definitions found: {result['board_count']}")

    must = result.get("must_define", [])
    never = result.get("never_defined", [])
    blk = result.get("block_info", {})

    print()
    log_info(f"=== Suggested defines for embedded-cmake.json ===")
    if must:
        print()
        print('Copy the following into the "defines" array:')
        print()
        for flag, reason in must:
            print(f'    "{flag}=1",')
        print()
        log_info(f"Total: {len(must)} suggested define(s)")
    else:
        log_info("  No additional defines suggested.")

    if never:
        print()
        log_info("=== Referenced but never defined (needs manual review) ===")
        print()
        for flag, reason in never:
            print(f"    {flag}")
            print(f"        {reason}")
        print()
        log_info(f"Total: {len(never)} unreferenced flag(s)")

    if blk:
        print()
        log_info("=== Board block summary ===")
        print(f"  Flags set to 1:        {blk['enabled_1_count']}")
        if blk["enabled_1"]:
            print(f"    {', '.join(blk['enabled_1'][:10])}"
                  f"{'...' if len(blk['enabled_1']) > 10 else ''}")
        print(f"  Flags set to 0:        {blk['enabled_0_count']}")
        print(f"  Commented-out flags:   {blk['commented_count']}")

    print()


def _print_dry_run_details(config, scan, hdr_dirs) -> None:
    """Print detailed configuration for dry-run mode."""
    print()
    log_info("=== Configuration Details ===")
    print(f"  Output directory: {config.cmake_dir}")
    print(f"  Defines: {' '.join(config.defines)}")
    print(f"  Startup file: {config.startup_file or '(auto)'}")
    print(f"  Linker script: {config.linker_script or '(auto)'}")
    if config.armcc_scatter_file:
        print(f"  Scatter file: {config.armcc_scatter_file}")
    if config.rtos.type != "none":
        print(f"  RTOS heap: {config.rtos.heap}")
        print(f"  RTOS CMSIS: {config.rtos.cmsis_version or 'N/A'}")
    print(f"  Exclude dirs: {config.scan.exclude_dirs[:5]}...")
    print(f"  Exclude files: {config.scan.exclude_files}")
    print()
    print("  First 10 C sources:")
    for s in scan.c_sources[:10]:
        print(f"    {os.path.basename(s)}")
    print()
    print("  Header directories:")
    for d in hdr_dirs[:10]:
        print(f"    {os.path.basename(d) if d else d}")


if __name__ == "__main__":
    sys.exit(main())
