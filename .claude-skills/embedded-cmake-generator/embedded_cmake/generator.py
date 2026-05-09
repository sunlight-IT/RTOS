"""CMake file generator.

Generates CMakeLists.txt, toolchain config files, and project config
from a ProjectConfig. Uses a structured builder approach instead of
string concatenation.
"""

from __future__ import annotations

import os
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional

from .models import (
    ProjectConfig, ChipInfo, ToolchainConfig, ScanResult,
    RTOSConfig, PostBuildStep,
)


class _CmakeWriter:
    """Helper to write formatted CMake output."""

    def __init__(self):
        self._lines: List[str] = []
        self._indent = 0

    def line(self, text: str = "") -> None:
        """Add a line with current indentation."""
        if text:
            self._lines.append("    " * self._indent + text)
        else:
            self._lines.append("")

    def comment(self, text: str) -> None:
        """Add a comment line."""
        self.line(f"# {text}")

    def indent(self) -> None:
        """Increase indent level."""
        self._indent += 1

    def dedent(self) -> None:
        """Decrease indent level."""
        if self._indent > 0:
            self._indent -= 1

    def set_list(self, name: str, items: List[str]) -> None:
        """Write a CMake set() with a list of values."""
        self.line(f"set({name}")
        self.indent()
        for item in items:
            self.line(item)
        self.dedent()
        self.line(")")

    def if_armcc(self) -> None:
        self.line('if(CMAKE_C_COMPILER_ID MATCHES "ARMCC")')

    def else_armcc(self) -> None:
        self.line("else()")

    def endif(self) -> None:
        self.line("endif()")

    def render(self) -> str:
        return "\n".join(self._lines) + "\n"


def generate_project_config(w: _CmakeWriter, config: ProjectConfig, scan: ScanResult, header_dirs: List[str]) -> None:
    """Generate project_config.cmake content."""
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    w.comment(f"Embedded Project Configuration")
    w.comment(f"Auto-generated: {now}")
    w.line()

    # Project name
    w.line(f'set(PROJECT_NAME "{config.project_name}")')
    w.line()

    # C sources
    w.comment("C source files")
    rel_c = [_source_rel_path(s, config.project_dir) for s in scan.c_sources]
    w.set_list("C_SOURCES", [f"${{CMAKE_CURRENT_SOURCE_DIR}}/{p}" for p in rel_c])
    w.line()

    # C++ sources
    w.comment("C++ source files")
    rel_cpp = [_source_rel_path(s, config.project_dir) for s in scan.cpp_sources]
    if rel_cpp:
        w.set_list("CPP_SOURCES", [f"${{CMAKE_CURRENT_SOURCE_DIR}}/{p}" for p in rel_cpp])
    else:
        w.line("set(CPP_SOURCES")
        w.line(")")
    w.line()

    # ASM sources - conditional on compiler
    w.comment("Assembly source files (compiler-dependent)")
    w.if_armcc()
    w.line("    # ARMCC: use arm subdirectory startup file")
    w.line("    set(ASM_SOURCES")
    if config.armcc_startup_file:
        w.line(f"        ${{CMAKE_CURRENT_SOURCE_DIR}}/{_norm_path(config.armcc_startup_file)}")
    w.line("    )")
    w.else_armcc()
    w.line("    # GCC: use project root startup file")
    w.line("    set(ASM_SOURCES")
    rel_asm = [_source_rel_path(s, config.project_dir) for s in scan.asm_sources]
    for p in rel_asm:
        w.line(f"        ${{CMAKE_CURRENT_SOURCE_DIR}}/{p}")
    w.line("    )")
    w.endif()
    w.line()

    # Include directories
    w.comment("Header include directories")
    # Filter out FreeRTOS port dirs (added conditionally later)
    filtered_headers = []
    for d in header_dirs:
        rel = _norm_path(os.path.relpath(d, config.project_dir))
        if "portable/GCC" not in rel and "portable/RVDS" not in rel:
            filtered_headers.append(f"${{CMAKE_CURRENT_SOURCE_DIR}}/{rel}")

    w.set_list("INCLUDE_DIRS", filtered_headers)
    w.line()

    # Conditional FreeRTOS port include dirs
    w.comment("FreeRTOS port include directories (compiler-dependent)")
    freertos_port_gcc = _find_freertos_port_path(config, "gcc")
    freertos_port_armcc = _find_freertos_port_path(config, "armcc")

    if freertos_port_armcc or freertos_port_gcc:
        w.if_armcc()
        w.line("    list(APPEND INCLUDE_DIRS")
        if freertos_port_armcc:
            w.line(f"        ${{CMAKE_CURRENT_SOURCE_DIR}}/{freertos_port_armcc}")
        w.line("    )")
        w.else_armcc()
        w.line("    list(APPEND INCLUDE_DIRS")
        if freertos_port_gcc:
            w.line(f"        ${{CMAKE_CURRENT_SOURCE_DIR}}/{freertos_port_gcc}")
        w.line("    )")
        w.endif()
    w.line()

    # Linker script
    w.comment("Linker script (compiler-dependent)")
    w.if_armcc()
    w.comment("ARMCC: use scatter file")
    if config.armcc_scatter_file:
        w.line(f'    set(LINKER_SCRIPT "${{CMAKE_CURRENT_SOURCE_DIR}}/{_norm_path(config.armcc_scatter_file)}")')
    else:
        w.line('    set(LINKER_SCRIPT "")')
    w.else_armcc()
    w.comment("GCC: use linker script")
    if config.linker_script:
        w.line(f'    set(LINKER_SCRIPT "${{CMAKE_CURRENT_SOURCE_DIR}}/{config.linker_script}")')
    else:
        w.line('    set(LINKER_SCRIPT "")')
    w.endif()


def generate_armcc_toolchain(w: _CmakeWriter, config: ProjectConfig) -> None:
    """Generate armcc-toolchain.cmake content."""
    armcc_dir = config.armcc_compiler_dir or "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin"

    w.line("# ARMCC toolchain config (Keil MDK)")
    w.line("# Uses forward-slash paths for bash compatibility")
    w.line()
    w.line("set(CMAKE_SYSTEM_NAME Generic)")
    w.line("set(CMAKE_SYSTEM_PROCESSOR ARM)")
    w.line()
    w.comment("Tool paths - adjust to your Keil installation")
    w.line(f'set(CMAKE_C_COMPILER "{armcc_dir}/armcc.exe")')
    w.line(f'set(CMAKE_CXX_COMPILER "{armcc_dir}/armcc.exe")')
    w.line(f'set(CMAKE_ASM_COMPILER "{armcc_dir}/armasm.exe")')
    w.line(f'set(CMAKE_OBJCOPY "{armcc_dir}/fromelf.exe")')
    w.line(f'set(CMAKE_SIZE "{armcc_dir}/fromelf.exe")')
    w.line(f'set(CMAKE_LINKER "{armcc_dir}/armlink.exe")')
    w.line()
    w.comment("Output suffix")
    w.line("set(CMAKE_EXECUTABLE_SUFFIX .axf)")
    w.line("set(CMAKE_LINK_LIBRARY_SUFFIX .a)")
    w.line("set(CMAKE_STATIC_LIBRARY_SUFFIX .a)")
    w.line()
    w.comment("Avoid scatter file during compiler test (critical)")
    w.line("set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)")
    w.line()

    # CPU flags - resolve from chip info and toolchain
    cpu_flags = _resolve_armcc_cpu_flags(config)
    w.comment(f"CPU configuration ({config.chip_info.cpu.core if config.chip_info else 'Cortex-M3'})")
    w.line(f'set(CPU_FLAGS "{cpu_flags}")')
    w.line()

    # Compiler flags
    w.line('set(CMAKE_C_FLAGS_INIT " ")')
    w.line('string(APPEND CMAKE_C_FLAGS_INIT "${CPU_FLAGS} --c99")')
    w.line('string(APPEND CMAKE_C_FLAGS_DEBUG_INIT "-Otime -g")')
    w.line('string(APPEND CMAKE_C_FLAGS_RELEASE_INIT "-Otime --g")')
    w.line()
    w.line('set(CMAKE_CXX_FLAGS_INIT " ")')
    w.line('string(APPEND CMAKE_CXX_FLAGS_INIT "${CPU_FLAGS} --cpp --no_exceptions")')
    w.line('string(APPEND CMAKE_CXX_FLAGS_DEBUG_INIT "-Otime -g")')
    w.line('string(APPEND CMAKE_CXX_FLAGS_RELEASE_INIT "-Otime --g")')
    w.line()
    w.line('set(CMAKE_ASM_FLAGS_INIT " ")')
    w.line('string(APPEND CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS}")')
    w.line()

    # Linker flags
    scatter_path = _norm_path(config.armcc_scatter_file if config.armcc_scatter_file else "MDK-ARM/Project/Project.sct")
    w.comment("Linker options with scatter file")
    w.line('set(CMAKE_EXE_LINKER_FLAGS_INIT " ")')
    w.line(f'string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT "--scatter ${{CMAKE_CURRENT_SOURCE_DIR}}/{scatter_path} --entry Reset_Handler --list --map --xref --callgraph --symbols --info sizes --info totals --info unused --info veneers")')
    w.line('string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT "--debug")')
    w.line()

    # Preprocessor defines
    defines_str = " ".join(f"-D{d}" for d in config.defines)
    extra_defines = " ".join(f"-D{d}" for d in config.extra_defines)
    # Add ZTHREAD check (project-specific)
    all_defines = f"{defines_str} {extra_defines} -DNO_USE_ZTHREAD_CHECK".strip()
    w.line("# Preprocessor defines")
    w.line(f"add_definitions({all_defines})")


def generate_gcc_toolchain(w: _CmakeWriter, config: ProjectConfig) -> None:
    """Generate arm-none-eabi-toolchain.cmake content."""
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
    w.line("set(CMAKE_EXECUTABLE_SUFFIX_ASM .elf)")
    w.line("set(CMAKE_EXECUTABLE_SUFFIX_C .elf)")
    w.line("set(CMAKE_EXECUTABLE_SUFFIX_CXX .elf)")
    w.line()
    w.line("set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)")
    w.line()

    # CPU flags - resolve from chip info
    cpu_flags = _resolve_gcc_cpu_flags(config)
    w.comment(f"CPU configuration ({config.chip_info.cpu.core if config.chip_info else 'Cortex-M3'})")
    w.line(f'set(CPU_FLAGS "{cpu_flags}")')
    w.line()

    w.line('set(CMAKE_C_FLAGS "${CPU_FLAGS} -Wall -Wextra -g -fdata-sections -ffunction-sections")')
    w.line('set(CMAKE_CXX_FLAGS "${CPU_FLAGS} -Wall -Wextra -g -fdata-sections -ffunction-sections")')
    w.line('set(CMAKE_ASM_FLAGS "${CPU_FLAGS} -x assembler-with-cpp")')
    w.line('set(CMAKE_EXE_LINKER_FLAGS "${CPU_FLAGS} -specs=nano.specs -Wl,--gc-sections")')
    w.line()
    w.comment("Debug: -O0")
    w.line('set(CMAKE_C_FLAGS_DEBUG "-O0 ${CMAKE_C_FLAGS}")')
    w.line('set(CMAKE_CXX_FLAGS_DEBUG "-O0 ${CMAKE_CXX_FLAGS}")')
    w.line()
    w.comment("Release: -O2")
    w.line('set(CMAKE_C_FLAGS_RELEASE "-O2 ${CMAKE_C_FLAGS}")')
    w.line('set(CMAKE_CXX_FLAGS_RELEASE "-O2 ${CMAKE_CXX_FLAGS}")')


def generate_cmake_lists(w: _CmakeWriter, config: ProjectConfig, output_dir: str) -> None:
    """Generate CMakeLists.txt content."""
    w.line(f"cmake_minimum_required(VERSION 3.20)")
    w.line(f"project({config.project_name} C CXX ASM)")
    w.line()
    w.comment("Load project configuration")
    w.line(f"include(${{CMAKE_CURRENT_SOURCE_DIR}}/{output_dir}/project_config.cmake)")
    w.line()

    # FreeRTOS port swap for ARMCC
    if config.rtos.type == "FreeRTOS":
        w.comment("Swap FreeRTOS port for ARMCC compiler")
        w.if_armcc()
        w.comment("ARMCC: use RVDS port, remove GCC port")
        freertos_gcc_port = _find_freertos_port_path(config, "gcc")
        freertos_armcc_port = _find_freertos_port_path(config, "armcc")
        if freertos_gcc_port:
            w.line(f"    list(REMOVE_ITEM C_SOURCES")
            w.line(f"        ${{CMAKE_CURRENT_SOURCE_DIR}}/{freertos_gcc_port}/port.c")
            w.line(f"    )")
        if freertos_armcc_port:
            w.line(f"    list(APPEND C_SOURCES")
            w.line(f"        ${{CMAKE_CURRENT_SOURCE_DIR}}/{freertos_armcc_port}/port.c")
            w.line(f"    )")
        w.endif()
        w.line()

    w.comment("Add include directories")
    w.line("include_directories(${INCLUDE_DIRS})")
    w.line()

    w.comment("Preprocessor definitions")
    defines_str = " ".join(f"-D{d}" for d in config.defines)
    w.line(f"add_definitions({defines_str})")
    w.line()

    w.comment("Create executable")
    w.line("add_executable(${PROJECT_NAME}.elf")
    w.line("    ${C_SOURCES}")
    w.line("    ${CPP_SOURCES}")
    w.line("    ${ASM_SOURCES}")
    w.line(")")
    w.line()

    # ARMCC specific
    w.comment("Toolchain-specific configuration")
    w.if_armcc()
    w.line("    # ARMCC: use fromelf and scatter file")
    w.line("    set_target_properties(${PROJECT_NAME}.elf PROPERTIES")
    w.line("        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}")
    w.line("        OUTPUT_NAME ${PROJECT_NAME}")
    w.line('        SUFFIX ".axf"')
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
    w.line("    # Link libraries")
    w.line("    target_link_libraries(${PROJECT_NAME}.elf")
    w.line("        c")
    w.line("        m")
    w.line("        nosys")
    w.line("    )")
    w.line()
    w.line("    # Post-build: generate .hex and .bin with objcopy")
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


def generate_all(
    project_dir: str,
    output_dir: str,
    config: ProjectConfig,
    scan: ScanResult,
    header_dirs: List[str],
) -> List[str]:
    """Generate all CMake configuration files.

    Returns list of generated file paths.
    """
    output_path = os.path.join(project_dir, output_dir)
    os.makedirs(output_path, exist_ok=True)
    generated: List[str] = []

    # 1. project_config.cmake
    w = _CmakeWriter()
    generate_project_config(w, config, scan, header_dirs)
    path = os.path.join(output_path, "project_config.cmake")
    _write_file(path, w.render())
    generated.append(path)

    # 2. armcc-toolchain.cmake
    w = _CmakeWriter()
    generate_armcc_toolchain(w, config)
    path = os.path.join(output_path, "armcc-toolchain.cmake")
    _write_file(path, w.render())
    generated.append(path)

    # 3. arm-none-eabi-toolchain.cmake
    w = _CmakeWriter()
    generate_gcc_toolchain(w, config)
    path = os.path.join(output_path, "arm-none-eabi-toolchain.cmake")
    _write_file(path, w.render())
    generated.append(path)

    # 4. CMakeLists.txt
    w = _CmakeWriter()
    generate_cmake_lists(w, config, output_dir)
    path = os.path.join(project_dir, "CMakeLists.txt")
    _write_file(path, w.render())
    generated.append(path)

    return generated


# --- Generator utility functions ---

def _source_rel_path(abs_path: str, project_dir: str) -> str:
    """Convert absolute source path to relative forward-slash path."""
    rel = os.path.relpath(abs_path, project_dir)
    return _norm_path(rel)


def _norm_path(path: str) -> str:
    """Normalize path to forward slashes."""
    return path.replace("\\", "/")


def _resolve_gcc_cpu_flags(config: ProjectConfig) -> str:
    """Resolve GCC CPU flags from chip info."""
    if config.chip_info:
        core = config.chip_info.cpu.core
        fpu = config.chip_info.cpu.fpu
        cpu_map = {
            "Cortex-M0": "-mcpu=cortex-m0 -mthumb",
            "Cortex-M0+": "-mcpu=cortex-m0plus -mthumb",
            "Cortex-M3": "-mcpu=cortex-m3 -mthumb",
            "Cortex-M4": "-mcpu=cortex-m4 -mthumb",
            "Cortex-M7": "-mcpu=cortex-m7 -mthumb",
            "Cortex-M23": "-mcpu=cortex-m23 -mthumb",
            "Cortex-M33": "-mcpu=cortex-m33 -mthumb",
        }
        fpu_map = {
            "none": "-mfloat-abi=soft",
            "softvfp": "-mfloat-abi=softfp",
        }
        cpu_flag = cpu_map.get(core, "-mcpu=cortex-m3 -mthumb")
        fpu_flag = fpu_map.get(fpu, "-mfloat-abi=soft")
        if fpu_flag:
            return f"{cpu_flag} {fpu_flag}"
        return cpu_flag
    return "-mcpu=cortex-m3 -mthumb -mfloat-abi=soft"


def _resolve_armcc_cpu_flags(config: ProjectConfig) -> str:
    """Resolve ARMCC CPU flags from chip info."""
    if config.chip_info:
        core = config.chip_info.cpu.core
        fpu = config.chip_info.cpu.fpu
        cpu_map = {
            "Cortex-M0": "--cpu=Cortex-M0",
            "Cortex-M0+": "--cpu=Cortex-M0+",
            "Cortex-M3": "--cpu=Cortex-M3",
            "Cortex-M4": "--cpu=Cortex-M4",
            "Cortex-M7": "--cpu=Cortex-M7",
            "Cortex-M23": "--cpu=Cortex-M23",
            "Cortex-M33": "--cpu=Cortex-M33",
        }
        fpu_map = {
            "none": "",
            "softvfp": "--fpu=SoftVFP",
        }
        cpu_flag = cpu_map.get(core, "--cpu=Cortex-M3")
        fpu_flag = fpu_map.get(fpu, "")
        if fpu_flag:
            return f"{cpu_flag} {fpu_flag}"
        return cpu_flag
    return "--cpu=Cortex-M3"


def _find_freertos_port_path(config: ProjectConfig, toolchain: str) -> str:
    """Find the FreeRTOS port include directory for a toolchain."""
    rtos = config.rtos
    if rtos.port_mapping and toolchain in rtos.port_mapping:
        return rtos.port_mapping[toolchain]

    # Legacy fallback: construct from known patterns
    if rtos.type == "FreeRTOS":
        # Try Middlewares path
        base = "Middlewares/Third_Party/FreeRTOS/Source/portable"
        cpu_arch = _cpu_to_freertos_arch(config)
        tc_dir = {"gcc": f"{base}/GCC/{cpu_arch}", "armcc": f"{base}/RVDS/{cpu_arch}"}
        return tc_dir.get(toolchain, "")

    return ""


def _cpu_to_freertos_arch(config: ProjectConfig) -> str:
    """Convert CPU core name to FreeRTOS port architecture name."""
    if not config.chip_info:
        return "ARM_CM3"
    core = config.chip_info.cpu.core
    core_map = {
        "Cortex-M0": "ARM_CM0",
        "Cortex-M0+": "ARM_CM0",
        "Cortex-M3": "ARM_CM3",
        "Cortex-M4": "ARM_CM4F",
        "Cortex-M7": "ARM_CM7",
        "Cortex-M23": "ARM_CM23",
        "Cortex-M33": "ARM_CM33",
    }
    return core_map.get(core, "ARM_CM3")


def _write_file(path: str, content: str) -> None:
    """Write content to a file."""
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(content)
