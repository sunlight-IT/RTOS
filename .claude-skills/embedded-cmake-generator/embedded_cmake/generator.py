"""CMake file generator.

Generates CMakeLists.txt, toolchain config files, and project config
from a ``ProjectConfig``. Uses ``CmakeWriter`` for structured output.
"""

from __future__ import annotations

import json
import os
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List, Optional

from .cmake_writer import CmakeWriter
from .models import ProjectConfig, ScanResult
from .toolchain import ToolchainRegistry
from .utils import norm_path, source_rel_path, write_file


# ---------------------------------------------------------------------------
# JSON data loading helpers
# ---------------------------------------------------------------------------


def _data_dir() -> str:
    """Get the data directory path."""
    return os.path.join(os.path.dirname(__file__), "data")


def _load_json_data(filename: str) -> Dict[str, Any]:
    """Load a JSON file from the data directory. Returns empty dict on failure."""
    path = os.path.join(_data_dir(), filename)
    if not os.path.isfile(path):
        return {}
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
        return data if isinstance(data, dict) else {}
    except (OSError, json.JSONDecodeError):
        return {}


def _load_rtos_config() -> Dict[str, Any]:
    """Load RTOS configuration from data/rtos_config.json."""
    return _load_json_data("rtos_config.json")


# ---------------------------------------------------------------------------
# Path to the standalone wrapper script
# ---------------------------------------------------------------------------

_WRAPPER_PATH = Path(__file__).resolve().parent / "wrappers" / "armcc-relpath-wrapper.py"


def _read_wrapper_content() -> str:
    """Read the ARMCC relpath wrapper script from disk."""
    return _WRAPPER_PATH.read_text(encoding="utf-8")


# ---------------------------------------------------------------------------
# FreeRTOS / uCOS-II helper constants
# ---------------------------------------------------------------------------

# Module-level registry for arch mapping resolution (set by generate_all)
_tc_registry: ToolchainRegistry | None = None


def _set_arch_mappings(registry: Optional[ToolchainRegistry]) -> None:
    """Set the toolchain registry for arch mapping resolution."""
    global _tc_registry
    _tc_registry = registry


def _get_tc_attr(tc_id: str, attr: str, default: str = "") -> str:
    """Read a toolchain attribute from the JSON registry."""
    if _tc_registry:
        tc = _tc_registry.get(tc_id)
        if tc:
            return getattr(tc, attr, default)
    return default


def _get_tc_flags(tc_id: str, flag_key: str,
                  default: Optional[List[str]] = None) -> List[str]:
    """Read toolchain flags by key from the JSON registry."""
    if _tc_registry:
        tc = _tc_registry.get(tc_id)
        if tc and flag_key in tc.flags:
            return tc.flags[flag_key]
    return default or []


def _arch_from_registry(rtos: str, core: str, default: str) -> str:
    """Resolve an architecture name from toolchain registry data.

    Falls back to *default* if no registry is set or the registry
    doesn't have the requested mapping.
    """
    if _tc_registry:
        for tc_id in _tc_registry.list_ids():
            tc = _tc_registry.get(tc_id)
            if tc and rtos in tc.arch_mappings:
                mapping = tc.arch_mappings[rtos]
                if core in mapping:
                    return mapping[core]
    return default

def _get_ucos_tc_dir(toolchain: str, default: str = "RealView") -> str:
    """Get uCOS-II toolchain directory name from JSON config."""
    return _load_rtos_config().get("ucos", {}).get("tc_dirs", {}).get(toolchain, default)

# ---------------------------------------------------------------------------
# Content-generation functions
# ---------------------------------------------------------------------------

def generate_project_config(
    w: CmakeWriter,
    config: ProjectConfig,
    scan: ScanResult,
    header_dirs: List[str],
) -> None:
    """Generate ``project_config.cmake`` content."""
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    w.comment(f"Embedded Project Configuration")
    w.comment(f"Auto-generated: {now}")
    w.line()

    w.line(f'set(PROJECT_NAME "{config.project_name}")')
    w.line()

    # C sources
    w.comment("C source files")
    rel_c = [source_rel_path(s, config.project_dir) for s in scan.c_sources]
    w.set_list("C_SOURCES", [f"${{CMAKE_CURRENT_SOURCE_DIR}}/{p}" for p in rel_c])
    w.line()

    # Monolithic include annotation
    if config.monolithic and config.monolithic.leaf_files:
        w.comment("Monolithic includes detected:")
        w.comment("  The following .c files are included by other .c files")
        w.comment("  and are NOT compiled separately:")
        for leaf in config.monolithic.leaf_files[:10]:
            rel_l = norm_path(os.path.relpath(leaf, config.project_dir))
            w.comment(f"    {rel_l}")
        if len(config.monolithic.leaf_files) > 10:
            w.comment(f"    ... and {len(config.monolithic.leaf_files) - 10} more")
    w.line()

    # C++ sources
    w.comment("C++ source files")
    rel_cpp = [source_rel_path(s, config.project_dir) for s in scan.cpp_sources]
    if rel_cpp:
        w.set_list("CPP_SOURCES", [f"${{CMAKE_CURRENT_SOURCE_DIR}}/{p}" for p in rel_cpp])
    else:
        w.line("set(CPP_SOURCES")
        w.line(")")
    w.line()

    # ASM sources — compiler-dependent
    w.comment("Assembly source files (compiler-dependent)")
    w.if_armcc()
    w.line("    # ARMCC: use arm directory startup file + uCOS-II .asm files")
    w.line("    set(ASM_SOURCES")
    if config.armcc_startup_file:
        w.line(f"        ${{CMAKE_CURRENT_SOURCE_DIR}}/{norm_path(config.armcc_startup_file)}")
    rel_asm = [source_rel_path(s, config.project_dir) for s in scan.asm_sources]
    for p in rel_asm:
        basename = os.path.basename(p)
        if config.armcc_startup_file and basename.startswith("startup_"):
            continue
        w.line(f"        ${{CMAKE_CURRENT_SOURCE_DIR}}/{p}")
    w.line("    )")
    w.else_armcc()
    w.line("    # GCC: use gcc-compatible startup file")
    w.line("    set(ASM_SOURCES")
    if config.startup_file:
        w.line(f"        ${{CMAKE_CURRENT_SOURCE_DIR}}/{norm_path(config.startup_file)}")
    rel_asm = [source_rel_path(s, config.project_dir) for s in scan.asm_sources]
    for p in rel_asm:
        basename = os.path.basename(p)
        # Skip ARMCC-specific startup files (MDK-ARM/ or arm/ directory versions)
        if basename.startswith("startup_") and (
            config.armcc_startup_file and basename in config.armcc_startup_file
        ):
            continue
        w.line(f"        ${{CMAKE_CURRENT_SOURCE_DIR}}/{p}")
    w.line("    )")
    w.endif()
    w.line()

    # Include directories
    w.comment("Header include directories")
    filtered_headers = []
    for d in header_dirs:
        rel = norm_path(os.path.relpath(d, config.project_dir))
        if "portable/GCC" not in rel and "portable/RVDS" not in rel:
            filtered_headers.append(f"${{CMAKE_CURRENT_SOURCE_DIR}}/{rel}")

    # Reorder: uCOS-II CPU/port paths must come FIRST to prevent
    # other libraries (e.g. lwip's arch/cpu.h) from shadowing cpu.h.
    priority_keywords = ["uC-CPU", "uC-CORE", "uC-LIB", "uC-PORT"]
    t1 = [h for h in filtered_headers if any(kw in h for kw in priority_keywords)]

    # Tier 2: Active chip family paths (prevent cross-MCU header conflicts).
    # Strip trailing digits so "STM32L1" matches dirs named "STM32L".
    remaining = [h for h in filtered_headers if h not in t1]
    chip_lower = (config.chip_family or "").lower().rstrip("0123456789")
    t2 = [h for h in remaining if chip_lower and chip_lower in h.lower()]

    # Tier 3: Everything else
    t3 = [h for h in remaining if h not in t2]
    filtered_headers = t1 + t2 + t3
    w.set_list("INCLUDE_DIRS", filtered_headers)
    w.line()

    # FreeRTOS port include dirs
    freertos_port_gcc = _find_freertos_port_path(config, "gcc")
    freertos_port_armcc = _find_freertos_port_path(config, "armcc")
    if freertos_port_armcc or freertos_port_gcc:
        w.if_armcc()
        if freertos_port_armcc:
            w.line(f"    list(APPEND INCLUDE_DIRS ${{CMAKE_CURRENT_SOURCE_DIR}}/{freertos_port_armcc})")
        w.else_armcc()
        if freertos_port_gcc:
            w.line(f"    list(APPEND INCLUDE_DIRS ${{CMAKE_CURRENT_SOURCE_DIR}}/{freertos_port_gcc})")
        w.endif()
    w.line()

    # Linker script
    w.comment("Linker script (compiler-dependent)")
    w.if_armcc()
    if config.armcc_scatter_file:
        w.line(f'    set(LINKER_SCRIPT "${{CMAKE_CURRENT_SOURCE_DIR}}/{norm_path(config.armcc_scatter_file)}")')
    else:
        w.line('    set(LINKER_SCRIPT "")')
    w.else_armcc()
    if config.linker_script:
        w.line(f'    set(LINKER_SCRIPT "${{CMAKE_CURRENT_SOURCE_DIR}}/{norm_path(config.linker_script)}")')
    else:
        w.line('    set(LINKER_SCRIPT "")')
    w.endif()


def generate_armcc_toolchain(w: CmakeWriter, config: ProjectConfig) -> None:
    """Generate ``armcc-toolchain.cmake`` content."""
    armcc_dir = config.armcc_compiler_dir
    if not armcc_dir:
        # Keep user-editable placeholder
        armcc_dir = "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin"

    w.line("# ARMCC toolchain config (Keil MDK)")
    w.line("# Uses forward-slash paths for bash compatibility")
    w.line()
    w.line("set(CMAKE_SYSTEM_NAME Generic)")
    w.line("set(CMAKE_SYSTEM_PROCESSOR ARM)")
    w.line()
    w.comment("Tool paths — adjust to your Keil installation")
    w.line(f'set(CMAKE_C_COMPILER "{armcc_dir}/armcc.exe")')
    w.line(f'set(CMAKE_CXX_COMPILER "{armcc_dir}/armcc.exe")')
    w.line(f'set(CMAKE_ASM_COMPILER "{armcc_dir}/armasm.exe")')
    w.line(f'set(CMAKE_OBJCOPY "{armcc_dir}/fromelf.exe")')
    w.line(f'set(CMAKE_SIZE "{armcc_dir}/fromelf.exe")')
    w.line(f'set(CMAKE_LINKER "{armcc_dir}/armlink.exe")')
    w.line()
    _suffix = _get_tc_attr("armcc", "executable_suffix", ".axf")
    _lib_sfx = _get_tc_attr("armcc", "link_library_suffix", ".a")
    _static_sfx = _get_tc_attr("armcc", "static_library_suffix", ".a")
    _try_target = _get_tc_attr("armcc", "try_compile_target_type", "STATIC_LIBRARY")
    w.line(f"set(CMAKE_EXECUTABLE_SUFFIX {_suffix})")
    w.line(f"set(CMAKE_LINK_LIBRARY_SUFFIX {_lib_sfx})")
    w.line(f"set(CMAKE_STATIC_LIBRARY_SUFFIX {_static_sfx})")
    w.line()
    w.comment("Avoid scatter file during compiler test (critical)")
    w.line(f"set(CMAKE_TRY_COMPILE_TARGET_TYPE {_try_target})")
    w.line()

    cpu_flags = _resolve_armcc_cpu_flags(config)
    w.comment(f"CPU configuration ({config.chip_info.cpu.core if config.chip_info else 'Cortex-M3'})")
    w.line(f'set(CPU_FLAGS "{cpu_flags}")')
    w.line()

    # Build C flags from Keil .uvprojx <Cads> options (detected per-project)
    std_flag = "--c99" if "99" in config.compiler_standard else ""
    gnu_flag = " --gnu" if config.armcc_gnu_mode else ""
    misc_cflag = f" {config.armcc_misc_cflags}" if config.armcc_misc_cflags else ""
    optim = config.armcc_optimization if config.armcc_optimization else "-Otime"
    w.line('set(CMAKE_C_FLAGS_INIT " ")')
    w.line(f'string(APPEND CMAKE_C_FLAGS_INIT "${{CPU_FLAGS}} {std_flag} -g{gnu_flag}{misc_cflag}")')
    w.line(f'string(APPEND CMAKE_C_FLAGS_DEBUG_INIT "{optim}")')
    w.line(f'string(APPEND CMAKE_C_FLAGS_RELEASE_INIT "{optim}")')
    w.line()
    w.line('set(CMAKE_CXX_FLAGS_INIT " ")')
    _cxx_flags = _get_tc_flags("armcc", "cxx_common", ["--cpp", "--no_exceptions"])
    _cxx_str = " ".join(_cxx_flags)
    w.line(f'string(APPEND CMAKE_CXX_FLAGS_INIT "${{CPU_FLAGS}} {_cxx_str} -g")')
    w.line(f'string(APPEND CMAKE_CXX_FLAGS_DEBUG_INIT "{optim}")')
    w.line(f'string(APPEND CMAKE_CXX_FLAGS_RELEASE_INIT "{optim}")')
    w.line()
    w.line('# Use relative-path wrapper so DWARF contains relative paths')
    w.line('# instead of absolute Windows paths (matches Keil MDK behaviour).')
    w.line('set(CMAKE_C_COMPILE_OBJECT "python ${CMAKE_CURRENT_LIST_DIR}/armcc-relpath-wrapper.py <CMAKE_C_COMPILER> <FLAGS> <DEFINES> <INCLUDES> -o <OBJECT> -c <SOURCE>")')
    w.line('set(CMAKE_CXX_COMPILE_OBJECT "python ${CMAKE_CURRENT_LIST_DIR}/armcc-relpath-wrapper.py <CMAKE_CXX_COMPILER> <FLAGS> <DEFINES> <INCLUDES> -o <OBJECT> -c <SOURCE>")')
    w.line()
    w.line('set(CMAKE_ASM_FLAGS_INIT " ")')
    w.line('string(APPEND CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS}")')
    if config.armcc_asm_defines:
        asm_pd = " ".join(f'--pd \\"{d} SETA 1\\"' for d in config.armcc_asm_defines)
        w.line(f'string(APPEND CMAKE_ASM_FLAGS_INIT " {asm_pd}")')
    if config.uses_microlib:
        w.line('# Enable Microlib for assembly (must match linker --library_type=microlib)')
        w.line('string(APPEND CMAKE_ASM_FLAGS_INIT " --pd \\"__MICROLIB SETA 1\\"")')
    w.line()

    scatter_path = config.armcc_scatter_file
    if not scatter_path:
        scatter_path = "MDK-ARM/Project/Project.sct"
        w.comment("WARN: scatter file not configured, using fallback MDK-ARM/Project/Project.sct")
    w.comment("Linker options with scatter file")
    misc_ldflag = f" {config.armcc_misc_ldflags}" if config.armcc_misc_ldflags else ""
    w.line('set(CMAKE_EXE_LINKER_FLAGS_INIT " ")')
    microlib_flag = " --library_type=microlib" if config.uses_microlib else ""
    w.line(f'string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT "--scatter ${{CMAKE_CURRENT_SOURCE_DIR}}/{norm_path(scatter_path)} --entry Reset_Handler --list --map --xref --callgraph --symbols --info sizes --info totals --info unused --info veneers --debug{microlib_flag}{misc_ldflag}")')
    w.line()
    w.line('# Use armlink.exe for linking even when CMake detects ASM as link language')
    w.line('# (default CMAKE_ASM_LINK_EXECUTABLE uses armasm.exe, which rejects --scatter)')
    w.line('set(CMAKE_ASM_LINK_EXECUTABLE "<CMAKE_LINKER> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")')
    w.line()

    # Preprocessor defines
    defines_str = " ".join(f"-D{d}" for d in config.defines)
    extra_defines = " ".join(f"-D{d}" for d in config.extra_defines)
    all_defines = f"{defines_str} {extra_defines}".strip()
    w.line("# Preprocessor defines")
    w.line(f"add_definitions({all_defines})")


def generate_gcc_toolchain(w: CmakeWriter, config: ProjectConfig) -> None:
    """Generate ``arm-none-eabi-toolchain.cmake`` content."""
    w.line("# ARM GCC toolchain config")
    w.line()
    w.line("set(CMAKE_SYSTEM_NAME Generic)")
    w.line("set(CMAKE_SYSTEM_PROCESSOR ARM)")
    w.line()
    w.line("set(TOOLCHAIN_PREFIX arm-none-eabi-)")
    w.line()
    w.line("find_program(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)")
    w.line("find_program(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)")
    w.line("find_program(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)")
    w.line("find_program(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)")
    w.line("find_program(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}objdump)")
    w.line("find_program(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size)")
    w.line()
    _suffix = _get_tc_attr("gcc", "executable_suffix", ".elf")
    _try_target = _get_tc_attr("gcc", "try_compile_target_type", "STATIC_LIBRARY")
    w.line(f"set(CMAKE_EXECUTABLE_SUFFIX_ASM {_suffix})")
    w.line(f"set(CMAKE_EXECUTABLE_SUFFIX_C {_suffix})")
    w.line(f"set(CMAKE_EXECUTABLE_SUFFIX_CXX {_suffix})")
    w.line()
    w.line(f"set(CMAKE_TRY_COMPILE_TARGET_TYPE {_try_target})")
    w.line()

    cpu_flags = _resolve_gcc_cpu_flags(config)
    w.comment(f"CPU configuration ({config.chip_info.cpu.core if config.chip_info else 'Cortex-M3'})")
    w.line(f'set(CPU_FLAGS "{cpu_flags}")')
    w.line()

    _c_f = " ".join(_get_tc_flags("gcc", "c_common",
                     ["-Wall", "-Wextra", "-g", "-fdata-sections", "-ffunction-sections"]))
    _cxx_f = " ".join(_get_tc_flags("gcc", "cxx_common",
                       ["-Wall", "-Wextra", "-g", "-fdata-sections", "-ffunction-sections"]))
    _asm_f = " ".join(_get_tc_flags("gcc", "asm_common", ["-x", "assembler-with-cpp"]))
    _ld_f = " ".join(_get_tc_flags("gcc", "linker_common", ["-specs=nano.specs", "-Wl,--gc-sections"]))
    w.line(f'set(CMAKE_C_FLAGS "${{CPU_FLAGS}} {_c_f}")')
    w.line(f'set(CMAKE_CXX_FLAGS "${{CPU_FLAGS}} {_cxx_f}")')
    w.line(f'set(CMAKE_ASM_FLAGS "${{CPU_FLAGS}} {_asm_f}")')
    w.line(f'set(CMAKE_EXE_LINKER_FLAGS "${{CPU_FLAGS}} {_ld_f}")')
    w.line()
    w.comment("Debug: -O0")
    w.line('set(CMAKE_C_FLAGS_DEBUG "-O0 ${CMAKE_C_FLAGS}")')
    w.line('set(CMAKE_CXX_FLAGS_DEBUG "-O0 ${CMAKE_CXX_FLAGS}")')
    w.line()
    w.comment("Release: -O2")
    w.line('set(CMAKE_C_FLAGS_RELEASE "-O2 ${CMAKE_C_FLAGS}")')
    w.line('set(CMAKE_CXX_FLAGS_RELEASE "-O2 ${CMAKE_CXX_FLAGS}")')


def generate_cmake_lists(w: CmakeWriter, config: ProjectConfig, scan: ScanResult, output_dir: str) -> None:
    """Generate ``CMakeLists.txt`` content."""
    w.line("cmake_minimum_required(VERSION 3.20)")
    w.line(f"project({config.project_name} C CXX ASM)")
    w.line()
    w.comment("Load project configuration")
    w.line(f"include(${{CMAKE_CURRENT_SOURCE_DIR}}/{output_dir}/project_config.cmake)")
    w.line()

    # FreeRTOS port swap
    if config.rtos.type == "FreeRTOS":
        w.comment("Swap FreeRTOS port for compiler")
        freertos_gcc_port = _find_freertos_port_path(config, "gcc")
        freertos_armcc_port = _find_freertos_port_path(config, "armcc")
        w.if_armcc()
        if freertos_gcc_port:
            w.line(f"    list(REMOVE_ITEM C_SOURCES ${{CMAKE_CURRENT_SOURCE_DIR}}/{freertos_gcc_port}/port.c)")
        if freertos_armcc_port:
            w.line(f"    list(APPEND C_SOURCES ${{CMAKE_CURRENT_SOURCE_DIR}}/{freertos_armcc_port}/port.c)")
        w.else_armcc()
        if freertos_armcc_port:
            w.line(f"    list(REMOVE_ITEM C_SOURCES ${{CMAKE_CURRENT_SOURCE_DIR}}/{freertos_armcc_port}/port.c)")
        if freertos_gcc_port:
            w.line(f"    list(APPEND C_SOURCES ${{CMAKE_CURRENT_SOURCE_DIR}}/{freertos_gcc_port}/port.c)")
        w.endif()
        w.line()

    # uCOS-II port swap
    if config.rtos.type == "uCOS-II":
        w.comment("Swap uCOS-II port for ARMCC compiler")
        _write_ucos_port_swap(w, config)
        w.line()

    w.comment("Add include directories")
    w.line("include_directories(${INCLUDE_DIRS})")
    w.line()

    w.comment("Preprocessor definitions")
    defines_str = " ".join(f"-D{d}" for d in config.defines)
    if config.rtos.type == "uCOS-II":
        defines_str += " -DCPU_CFG_DATA_SIZE=CPU_WORD_SIZE_32 -DCPU_CFG_DATA_SIZE_MAX=CPU_WORD_SIZE_64"
    w.line(f"add_definitions({defines_str})")
    w.line()

    w.comment("Create executable")
    w.line("add_executable(${PROJECT_NAME}.elf")
    w.line("    ${C_SOURCES}")
    w.line("    ${CPP_SOURCES}")
    w.line("    ${ASM_SOURCES}")
    w.line(")")
    w.line()

    w.comment("Toolchain-specific configuration")
    w.if_armcc()
    w.line("    # ARMCC: use fromelf and scatter file")
    _suffix = _get_tc_attr("armcc", "executable_suffix", ".axf")
    w.line("    set_target_properties(${PROJECT_NAME}.elf PROPERTIES")
    w.line("        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}")
    w.line("        OUTPUT_NAME ${PROJECT_NAME}")
    w.line(f'        SUFFIX "{_suffix}"')
    w.line("    )")
    w.line()

    # ARMCC: pre-compiled .lib files passed directly to linker via target_link_options
    lib_rel_paths = [source_rel_path(lib, config.project_dir) for lib in scan.lib_files]
    if lib_rel_paths:
        w.line("    # Pre-compiled libraries (.lib files)")
        w.line("    target_link_options(${PROJECT_NAME}.elf PRIVATE")
        for rel_lib in lib_rel_paths:
            w.line(f"        ${{CMAKE_CURRENT_SOURCE_DIR}}/{rel_lib}")
        w.line("    )")
        w.line()

    w.line("    # Post-build: generate .bin and .hex with fromelf")
    w.line("    add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD")
    w.line("        COMMAND ${CMAKE_OBJCOPY} --bin --output ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.axf")
    w.line("        COMMAND ${CMAKE_OBJCOPY} --i32 --output ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.axf")
    w.line('        COMMENT "Generating .bin and .hex files"')
    w.line("    )")
    w.else_armcc()
    w.line("    # GCC: use linker script and objcopy")
    w.line("    if(DEFINED LINKER_SCRIPT)")
    w.line("        target_link_options(${PROJECT_NAME}.elf PRIVATE")
    w.line("            -T${LINKER_SCRIPT}")
    w.line("        )")
    w.line("    endif()")
    w.line()
    w.line("    target_link_libraries(${PROJECT_NAME}.elf")
    w.line("        c")
    w.line("        m")
    w.line("        nosys")
    w.line("    )")

    # GCC: pre-compiled libraries
    if lib_rel_paths:
        w.line()
        w.line("    target_link_libraries(${PROJECT_NAME}.elf")
        for rel_lib in lib_rel_paths:
            w.line(f"        ${{CMAKE_CURRENT_SOURCE_DIR}}/{rel_lib}")
        w.line("    )")

    w.line()
    w.line("    add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD")
    w.line("        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex")
    w.line("        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin")
    w.line("        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>")
    w.line('        COMMENT "Generating .hex and .bin files"')
    w.line("    )")
    w.line()
    w.line("    set_target_properties(${PROJECT_NAME}.elf PROPERTIES")
    w.line("        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}")
    w.line("        OUTPUT_NAME ${PROJECT_NAME}")
    w.line("    )")
    w.endif()
    w.line()

    w.comment("Print configuration summary")
    w.line('message(STATUS "Project Name: ${PROJECT_NAME}")')
    w.line('message(STATUS "Compiler ID: ${CMAKE_C_COMPILER_ID}")')
    w.line('message(STATUS "C Sources: ${C_SOURCES}")')
    w.line('message(STATUS "C++ Sources: ${CPP_SOURCES}")')
    w.line('message(STATUS "ASM Sources: ${ASM_SOURCES}")')
    w.line('message(STATUS "Include Dirs: ${INCLUDE_DIRS}")')


# ---------------------------------------------------------------------------
# Orchestration
# ---------------------------------------------------------------------------

def generate_all(
    project_dir: str,
    output_dir: str,
    config: ProjectConfig,
    scan: ScanResult,
    header_dirs: List[str],
    toolchain_registry: Optional[ToolchainRegistry] = None,
) -> List[str]:
    """Generate all CMake configuration files.

    Args:
        project_dir: Root project directory.
        output_dir: Output subdirectory name (e.g. "cmake").
        config: Detected project configuration.
        scan: Source file scan results.
        header_dirs: List of header include directories.
        toolchain_registry: Optional toolchain registry for resolving
            CPU/arch flags from data files instead of inline maps.

    Returns:
        List of generated file paths.
    """
    # Store registry for helper functions
    _set_arch_mappings(toolchain_registry)

    output_path = os.path.join(project_dir, output_dir)
    os.makedirs(output_path, exist_ok=True)
    generated: List[str] = []

    # 1. project_config.cmake
    w = CmakeWriter()
    generate_project_config(w, config, scan, header_dirs)
    path = os.path.join(output_path, "project_config.cmake")
    write_file(path, w.render())
    generated.append(path)

    # 2. armcc-toolchain.cmake
    w = CmakeWriter()
    generate_armcc_toolchain(w, config)
    path = os.path.join(output_path, "armcc-toolchain.cmake")
    write_file(path, w.render())
    generated.append(path)

    # 2b. armcc-relpath-wrapper.py
    wrapper_path = os.path.join(output_path, "armcc-relpath-wrapper.py")
    write_file(wrapper_path, _read_wrapper_content())
    generated.append(wrapper_path)

    # 3. arm-none-eabi-toolchain.cmake
    w = CmakeWriter()
    generate_gcc_toolchain(w, config)
    path = os.path.join(output_path, "arm-none-eabi-toolchain.cmake")
    write_file(path, w.render())
    generated.append(path)

    # 4. CMakeLists.txt
    w = CmakeWriter()
    generate_cmake_lists(w, config, scan, output_dir)
    path = os.path.join(project_dir, "CMakeLists.txt")
    write_file(path, w.render())
    generated.append(path)

    return generated


# ---------------------------------------------------------------------------
# CPU flag resolution (data-driven via toolchain registry)
# ---------------------------------------------------------------------------

def _resolve_gcc_cpu_flags(config: ProjectConfig) -> str:
    """Resolve GCC CPU+FPU flags from toolchain registry (data-driven)."""
    if not config.chip_info:
        return "-mcpu=cortex-m3 -mthumb -mfloat-abi=soft"
    if _tc_registry:
        gcc = _tc_registry.get("gcc")
        if gcc:
            result = gcc.resolve_cpu_flags(config.chip_info.cpu)
            if result:
                return result
    core = config.chip_info.cpu.core
    # Bare fallback when no registry
    cpu_flag = {"Cortex-M0": "-mcpu=cortex-m0 -mthumb",
                "Cortex-M0+": "-mcpu=cortex-m0plus -mthumb",
                "Cortex-M3": "-mcpu=cortex-m3 -mthumb",
                "Cortex-M4": "-mcpu=cortex-m4 -mthumb",
                "Cortex-M7": "-mcpu=cortex-m7 -mthumb"}.get(core, "-mcpu=cortex-m3 -mthumb")
    fpu = config.chip_info.cpu.fpu
    fpu_flag = "" if fpu in ("none", "") else "-mfloat-abi=soft"
    return f"{cpu_flag} {fpu_flag}" if fpu_flag else cpu_flag


def _resolve_armcc_cpu_flags(config: ProjectConfig) -> str:
    """Resolve ARMCC CPU+FPU flags from toolchain registry (data-driven)."""
    if not config.chip_info:
        return "--cpu=Cortex-M3"
    if _tc_registry:
        armcc = _tc_registry.get("armcc")
        if armcc:
            result = armcc.resolve_cpu_flags(config.chip_info.cpu)
            if result:
                return result
    core = config.chip_info.cpu.core
    cpu_flag = {"Cortex-M0": "--cpu=Cortex-M0",
                "Cortex-M3": "--cpu=Cortex-M3",
                "Cortex-M4": "--cpu=Cortex-M4"}.get(core, "--cpu=Cortex-M3")
    fpu = config.chip_info.cpu.fpu
    fpu_flag = {"softvfp": "--fpu=SoftVFP"}.get(fpu, "")
    return f"{cpu_flag} {fpu_flag}" if fpu_flag else cpu_flag


# ---------------------------------------------------------------------------
# FreeRTOS port resolution
# ---------------------------------------------------------------------------

def _cpu_to_freertos_arch(config: ProjectConfig) -> str:
    if not config.chip_info:
        return "ARM_CM3"
    core = config.chip_info.cpu.core
    # Try registry first (data-driven), fall back to Cortex-M3 default
    return _arch_from_registry("freertos", core, "ARM_CM3")


def _find_freertos_port_path(config: ProjectConfig, toolchain: str) -> str:
    rtos = config.rtos
    if rtos.port_mapping and toolchain in rtos.port_mapping:
        return norm_path(rtos.port_mapping[toolchain])
    if rtos.type == "FreeRTOS":
        base = "Middlewares/Third_Party/FreeRTOS/Source/portable"
        cpu_arch = _cpu_to_freertos_arch(config)
        tc_dir = {"gcc": f"{base}/GCC/{cpu_arch}", "armcc": f"{base}/RVDS/{cpu_arch}"}
        return tc_dir.get(toolchain, "")
    return ""


# ---------------------------------------------------------------------------
# uCOS-II port resolution
# ---------------------------------------------------------------------------

def _cpu_to_ucos_arch(config: ProjectConfig) -> str:
    if not config.chip_info:
        return "ARM-Cortex-M3"
    core = config.chip_info.cpu.core
    # Try registry first (data-driven), fall back to Cortex-M3 default
    return _arch_from_registry("ucos", core, "ARM-Cortex-M3")


def _find_ucos_cpu_asm(config: ProjectConfig, toolchain: str) -> str:
    rtos = config.rtos
    if not rtos.base_path:
        return ""
    tc_dir = _get_ucos_tc_dir(toolchain)
    cpu_arch = _cpu_to_ucos_arch(config)
    candidates = [
        f"{rtos.base_path}/uC-CPU/{cpu_arch}/cpu_a.asm",
        f"{rtos.base_path}/uC-CPU/{cpu_arch}/{tc_dir}/cpu_a.s",
        f"{rtos.base_path}/uC-CPU/{cpu_arch}/{tc_dir}/cpu_a.asm",
    ]
    for c in candidates:
        full = os.path.join(config.project_dir, c) if not os.path.isabs(c) else c
        if os.path.isfile(full):
            return c
    return ""


def _find_ucos_port_asm(config: ProjectConfig, toolchain: str) -> str:
    rtos = config.rtos
    if not rtos.base_path:
        return ""
    tc_dir = _get_ucos_tc_dir(toolchain)
    cpu_arch = _cpu_to_ucos_arch(config)
    candidates = [
        f"{rtos.base_path}/uC-PORT/{cpu_arch}/Generic/{tc_dir}/os_cpu_a.asm",
        f"{rtos.base_path}/uC-PORT/{cpu_arch}/Generic/{tc_dir}/os_cpu_a.s",
        f"{rtos.base_path}/uC-PORT/{cpu_arch}/Generic/{tc_dir}/os_cpu_a.S",
    ]
    for c in candidates:
        full = os.path.join(config.project_dir, c) if not os.path.isabs(c) else c
        if os.path.isfile(full):
            return c
    return ""


def _find_ucos_port_path(config: ProjectConfig, toolchain: str) -> str:
    rtos = config.rtos
    if rtos.port_mapping and toolchain in rtos.port_mapping:
        return norm_path(rtos.port_mapping[toolchain])
    if not rtos.base_path:
        return ""
    cpu_arch = _cpu_to_ucos_arch(config)
    tc_dir = {
        "gcc": f"uC-PORT/{cpu_arch}/Generic/GNU",
        "armcc": f"uC-PORT/{cpu_arch}/Generic/RealView",
        "iar": f"uC-PORT/{cpu_arch}/Generic/IAR",
    }
    suffix = tc_dir.get(toolchain, "")
    if not suffix:
        return ""
    # base_path is absolute; convert to project-relative
    rel_base = os.path.relpath(rtos.base_path, config.project_dir)
    return norm_path(os.path.join(rel_base, suffix))


def _write_ucos_port_swap(w: CmakeWriter, config: ProjectConfig) -> None:
    """Write uCOS-II port swap logic for ARMCC vs GCC (bidirectional)."""
    gnu_port = _find_ucos_port_path(config, "gcc")
    rvds_port = _find_ucos_port_path(config, "armcc")

    w.if_armcc()
    if gnu_port:
        w.line("    list(REMOVE_ITEM C_SOURCES")
        for file in ["os_cpu_c.c", "os_dbg.c", "cpu_c.c"]:
            full = os.path.join(config.project_dir, gnu_port, file)
            if os.path.isfile(full):
                w.line(f"        ${{CMAKE_CURRENT_SOURCE_DIR}}/{norm_path(gnu_port)}/{file}")
        w.line("    )")
    if rvds_port:
        w.line("    list(APPEND C_SOURCES")
        for file in ["os_cpu_c.c", "os_dbg.c", "cpu_c.c"]:
            full = os.path.join(config.project_dir, rvds_port, file)
            if os.path.isfile(full):
                w.line(f"        ${{CMAKE_CURRENT_SOURCE_DIR}}/{norm_path(rvds_port)}/{file}")
        w.line("    )")
    w.else_armcc()
    if rvds_port:
        w.line("    list(REMOVE_ITEM C_SOURCES")
        for file in ["os_cpu_c.c", "os_dbg.c", "cpu_c.c"]:
            full = os.path.join(config.project_dir, rvds_port, file)
            if os.path.isfile(full):
                w.line(f"        ${{CMAKE_CURRENT_SOURCE_DIR}}/{norm_path(rvds_port)}/{file}")
        w.line("    )")
    if gnu_port:
        w.line("    list(APPEND C_SOURCES")
        for file in ["os_cpu_c.c", "os_dbg.c", "cpu_c.c"]:
            full = os.path.join(config.project_dir, gnu_port, file)
            if os.path.isfile(full):
                w.line(f"        ${{CMAKE_CURRENT_SOURCE_DIR}}/{norm_path(gnu_port)}/{file}")
        w.line("    )")
    w.endif()
