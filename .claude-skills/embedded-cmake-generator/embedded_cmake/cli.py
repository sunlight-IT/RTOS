"""Command-line interface for the embedded CMake generator.

Usage:
    python -m embedded_cmake [options]
    python embedded_cmake/cli.py [options]
"""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path
from typing import List, Optional

from . import __version__
from .chip_db import ChipDB
from .toolchain import ToolchainRegistry
from .config import load_config
from .scanner import scan_sources, scan_headers
from .generator import generate_all


def log_info(msg: str) -> None:
    print(f"\033[0;32m[INFO]\033[0m {msg}")


def log_warn(msg: str) -> None:
    print(f"\033[1;33m[WARN]\033[0m {msg}")


def log_error(msg: str) -> None:
    print(f"\033[0;31m[ERROR]\033[0m {msg}")


def main(argv: Optional[List[str]] = None) -> int:
    """Main entry point. Returns exit code."""
    parser = _build_parser()
    args = parser.parse_args(argv)

    project_dir = os.path.abspath(args.project_dir)
    if not os.path.isdir(project_dir):
        log_error(f"Project directory not found: {project_dir}")
        return 1

    log_info(f"Embedded CMake Generator v{__version__}")
    log_info(f"Project directory: {project_dir}")

    # Load chip DB and toolchain registry
    chip_db = ChipDB()
    if args.chip_db_path:
        chip_db.add_search_path(Path(args.chip_db_path))
    chip_db.load_builtin()

    tc_registry = ToolchainRegistry()
    if args.toolchain_data_path:
        tc_registry.add_search_path(Path(args.toolchain_data_path))
    tc_registry.load_builtin()

    # Load configuration (auto-detect + optional config file)
    log_info("Detecting project configuration...")
    config = load_config(project_dir, chip_db, tc_registry)

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

    # Print detected config
    log_info(f"  Project: {config.project_name}")
    if config.chip_info:
        log_info(f"  Chip: {config.chip_info.family} / {config.chip_model} "
                 f"({config.chip_info.cpu.core})")
    log_info(f"  Toolchains: {', '.join(config.toolchains)}")
    if config.rtos.type != "none":
        log_info(f"  RTOS: {config.rtos.type}")
    if config.is_cubemx_project:
        log_info(f"  CubeMX: {os.path.basename(config.ioc_file)}")

    # Scan sources
    log_info("Scanning source files...")
    scan = scan_sources(project_dir, config.scan)
    hdr_dirs = scan_headers(project_dir, config.scan)

    log_info(f"  C sources: {len(scan.c_sources)}")
    log_info(f"  C++ sources: {len(scan.cpp_sources)}")
    log_info(f"  ASM sources: {len(scan.asm_sources)}")
    log_info(f"  Header dirs: {len(hdr_dirs)}")

    if args.dry_run:
        log_info("Dry run - skipping file generation.")
        _print_dry_run_details(config, scan, hdr_dirs)
        return 0

    # Generate CMake files
    log_info("Generating CMake files...")
    generated = generate_all(project_dir, config.cmake_dir, config, scan, hdr_dirs)

    log_info(f"Generated {len(generated)} files:")
    for f in generated:
        rel = os.path.relpath(f, project_dir)
        print(f"  - {rel}")

    print()
    log_info("Next steps:")
    _print_build_instructions(config)

    return 0


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
    parser.add_argument("--init", action="store_true",
                        help="Generate an embedded-cmake.json template file")
    parser.add_argument("--version", action="version",
                        version=f"embedded-cmake-generator v{__version__}")

    return parser


def _print_build_instructions(config: 'ProjectConfig') -> None:
    """Print build instructions for detected toolchains."""
    for tc_id in config.toolchains:
        tc_name = tc_id.upper()
        if tc_id == "gcc":
            print(f"  [{tc_name}]")
            print(f"    mkdir -p build && cd build")
            print(f"    cmake .. -G \"MinGW Makefiles\" "
                  f"-DCMAKE_TOOLCHAIN_FILE=../{config.cmake_dir}/arm-none-eabi-toolchain.cmake")
            print(f"    make -j4")
        elif tc_id == "armcc":
            print(f"  [{tc_name}]")
            print(f"    mkdir -p build && cd build")
            print(f"    cmake .. -G \"MinGW Makefiles\" "
                  f"-DCMAKE_TOOLCHAIN_FILE=../{config.cmake_dir}/armcc-toolchain.cmake")
            print(f"    make -j4")
            if config.armcc_compiler_dir:
                print(f"    (ARMCC compiler dir: {config.armcc_compiler_dir})")
    print()
    if "armcc" in config.toolchains:
        log_info("Note: ARMCC toolchain requires configuring the compiler path")
        log_info(f"  in {config.cmake_dir}/armcc-toolchain.cmake if not auto-detected")


def _print_dry_run_details(
    config: 'ProjectConfig',
    scan: 'ScanResult',
    hdr_dirs: List[str],
) -> None:
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
