"""Data models for the embedded CMake generator.

All configuration and state is represented as dataclasses for type safety
and clear data flow through the generator pipeline.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Dict, List, Optional


@dataclass
class MemoryRegion:
    """A memory region (FLASH, RAM, etc.) with origin and length."""
    name: str
    origin: str          # e.g. "0x08000000"
    length: str          # e.g. "64K" or "0x10000"


@dataclass
class CpuInfo:
    """CPU core and FPU information for a chip family."""
    core: str            # e.g. "Cortex-M3", "Cortex-M4"
    fpu: str = "none"    # e.g. "none", "fpv4-sp-d16", "softvfp"
    endian: str = "little"


@dataclass
class ChipModel:
    """Per-model information within a chip family."""
    name: str
    defines: List[str] = field(default_factory=list)
    memory: List[MemoryRegion] = field(default_factory=list)
    max_clock_hz: int = 0

    def get_memory(self, name: str) -> Optional[MemoryRegion]:
        for region in self.memory:
            if region.name.upper() == name.upper():
                return region
        return None


@dataclass
class StartFileInfo:
    """Startup file patterns and defaults for a chip family."""
    pattern: str = ""                  # glob pattern for gcc startup file
    gcc_default: str = ""             # default gcc startup filename
    armcc_pattern: str = ""           # pattern for armcc startup files
    armcc_default: str = ""           # default armcc startup file path
    iar_pattern: str = ""             # pattern for IAR startup files


@dataclass
class LinkerFileInfo:
    """Linker script / scatter file patterns for a chip family."""
    gcc_pattern: str = ""             # glob pattern for .ld files
    armcc_pattern: str = ""           # glob pattern for .sct files
    iar_pattern: str = ""             # glob pattern for .icf files


@dataclass
class ChipInfo:
    """Complete information about a chip family."""
    family: str                       # e.g. "STM32F1"
    vendor: str                       # e.g. "STMicroelectronics"
    cpu: CpuInfo = field(default_factory=lambda: CpuInfo(core=""))
    models: Dict[str, ChipModel] = field(default_factory=dict)
    default_defines: List[str] = field(default_factory=list)
    header_paths: Dict[str, str] = field(default_factory=dict)
    startup: StartFileInfo = field(default_factory=StartFileInfo)
    linker: LinkerFileInfo = field(default_factory=LinkerFileInfo)
    hal_driver_name: str = ""         # e.g. "STM32F1xx_HAL_Driver"
    ext: Dict = field(default_factory=dict)  # extensible vendor-specific data

    def get_model(self, model_name: str) -> Optional[ChipModel]:
        return self.models.get(model_name)

    def find_model_by_define(self, define: str) -> Optional[ChipModel]:
        for model in self.models.values():
            if define in model.defines:
                return model
        return None


@dataclass
class PostBuildStep:
    """A post-build command step."""
    command: List[str]
    comment: str = ""


@dataclass
class PostBuildConfig:
    """Post-build configuration for a toolchain."""
    hex: Optional[PostBuildStep] = None
    bin: Optional[PostBuildStep] = None
    size: Optional[PostBuildStep] = None


@dataclass
class ToolchainConfig:
    """Configuration for a specific toolchain."""
    name: str                         # e.g. "GCC", "ARMCC"
    toolchain_id: str                 # e.g. "gcc", "armcc"
    cmake_compiler_id: str            # e.g. "GNU", "ARMCC"
    compiler_method: str = "prefix"   # "prefix" or "absolute_path"

    # For prefix method (GCC, Clang)
    compiler_prefix: str = ""

    # For absolute path method (ARMCC, IAR)
    tools: Dict[str, str] = field(default_factory=dict)

    executable_suffix: str = ".elf"
    try_compile_target_type: str = "STATIC_LIBRARY"

    # CPU and FPU flag maps
    cpu_flags: Dict[str, str] = field(default_factory=dict)
    fpu_flags: Dict[str, str] = field(default_factory=dict)

    # Compile flags by category
    flags: Dict[str, List[str]] = field(default_factory=dict)

    # Linker
    link_libraries: List[str] = field(default_factory=list)
    linker_script_option_template: str = "-T{path}"

    # Post-build
    post_build: PostBuildConfig = field(default_factory=PostBuildConfig)

    # CMake target properties
    target_properties: Dict[str, str] = field(default_factory=dict)

    def resolve_cpu_flags(self, cpu: CpuInfo) -> str:
        """Resolve toolchain-specific CPU + FPU flags from CpuInfo."""
        cpu_flag = self.cpu_flags.get(cpu.core, "")
        fpu_flag = self.fpu_flags.get(cpu.fpu, "")
        if cpu_flag and fpu_flag:
            return f"{cpu_flag} {fpu_flag}"
        return cpu_flag or fpu_flag

    def resolve_tool_path(self, tool_name: str, compiler_dir: str = "") -> str:
        """Resolve a tool binary path."""
        if self.compiler_method == "prefix":
            return self.tools.get(tool_name, "").format(prefix=self.compiler_prefix)
        else:
            template = self.tools.get(tool_name, "")
            return template.format(compiler_dir=compiler_dir)


@dataclass
class RTOSConfig:
    """Detected or configured RTOS information."""
    type: str = "none"                # "none", "FreeRTOS", "ThreadX", "RTThread"
    heap: str = "heap_4.c"           # selected heap implementation
    cmsis_version: str = ""          # "V1", "V2", or empty
    base_path: str = ""              # path to RTOS root directory
    port_mapping: Dict[str, str] = field(default_factory=dict)  # toolchain -> port_dir


@dataclass
class ScanConfig:
    """Configurable scan rules for source/header discovery."""
    source_extensions: List[str] = field(default_factory=lambda: [
        ".c", ".cpp", ".cxx", ".cc", ".s", ".S"
    ])
    header_extensions: List[str] = field(default_factory=lambda: [
        ".h", ".hpp"
    ])
    exclude_dirs: List[str] = field(default_factory=list)
    exclude_dir_patterns: List[str] = field(default_factory=list)
    exclude_files: List[str] = field(default_factory=list)
    exclude_file_patterns: List[str] = field(default_factory=list)
    extra_exclude_header_dirs: List[str] = field(default_factory=list)


@dataclass
class ScanResult:
    """Result of scanning a project directory."""
    c_sources: List[str] = field(default_factory=list)
    cpp_sources: List[str] = field(default_factory=list)
    asm_sources: List[str] = field(default_factory=list)
    header_dirs: List[str] = field(default_factory=list)


@dataclass
class ProjectConfig:
    """Complete project configuration (all fields optional — auto-detected if missing)."""
    project_name: str = ""
    project_dir: str = "."

    # Chip
    chip_family: str = ""
    chip_model: str = ""
    chip_info: Optional[ChipInfo] = None

    # Scan
    scan: ScanConfig = field(default_factory=ScanConfig)

    # Defines
    defines: List[str] = field(default_factory=list)

    # Toolchains to generate configs for
    toolchains: List[str] = field(default_factory=lambda: ["gcc"])

    # ARMCC specific
    armcc_compiler_dir: str = ""

    # RTOS
    rtos: RTOSConfig = field(default_factory=RTOSConfig)

    # Files
    startup_file: str = ""
    armcc_startup_file: str = ""
    linker_script: str = ""
    armcc_scatter_file: str = ""

    # Output
    cmake_dir: str = "cmake"
    output_hex: bool = True
    output_bin: bool = True
    output_size: bool = True

    # Detected (filled by detector)
    is_cubemx_project: bool = False
    ioc_file: str = ""
    extra_defines: List[str] = field(default_factory=list)
