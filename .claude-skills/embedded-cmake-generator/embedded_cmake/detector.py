"""Project auto-detection orchestrator.

Detects project type, chip family/model, RTOS, toolchain, startup files,
and linker scripts by inspecting the project directory structure.

Detection proceeds in multiple passes:
1. CubeMX project (.ioc file)
2. Chip header detection
3. RTOS detection
4. Toolchain detection
5. Startup/linker file detection
6. Project name fallback
"""

from __future__ import annotations

import glob
import os
from pathlib import Path
from typing import List, Optional, Tuple, Set

from .models import ProjectConfig, ScanConfig, RTOSConfig, ChipInfo
from .chip_db import ChipDB
from .toolchain import ToolchainRegistry
from .parsers import ioc_parser, makefile_parser


# Default chip header detection patterns
_CHIP_HEADER_PATTERNS = [
    # STM32
    (r"stm32f(\d)xx\.h", "STM32F{0}"),
    (r"stm32l(\d)xx\.h", "STM32L{0}"),
    (r"stm32g(\d)xx\.h", "STM32G{0}"),
    (r"stm32h(\d)xx\.h", "STM32H{0}"),
    # GD32 (GigaDevice, STM32-like)
    (r"gd32f(\w+)\.h", "GD32F{0}"),
    # NXP
    (r"MKL(\w+)Z(\d+)\.h", "Kinetis_KL{0}"),
    (r"MK(\w+)\.h", "Kinetis_K{0}"),
    # TI
    (r"tm4c(\w+)\.h", "TM4C{0}"),
]

# Default FreeRTOS port directory patterns
_FREERTOS_PORT_DIRS = {
    "gcc": "portable/GCC",
    "armcc": "portable/RVDS",
    "iar": "portable/IAR",
}

# Default CMSIS-RTOS exclude directory
_CMSIS_RTOS_V1_DIR = "CMSIS_RTOS"

# Common default exclude directories for embedded projects
_DEFAULT_EXCLUDE_DIRS = {
    "build", "cmake-build", "Debug", "Release",
    ".git", ".vscode", ".idea", ".settings",
    "__pycache__", ".pytest_cache", ".mypy_cache",
    "MDK-ARM", "EWARM",
    "Template", "Templates", "Examples",
    "Core_A", "DSP", "DSP_Lib_TestSuite", "NN", "RTOS2",
    "RVDS",  # ARMCC port, handled conditionally
}

# Default files to exclude (always)
_DEFAULT_EXCLUDE_FILES = {
    "syscalls.c",
    "SEGGER_RTT_ASM_ARMv7M.S",
}

# Common default exclude file patterns
_DEFAULT_EXCLUDE_FILE_PATTERNS = [
    "*template*",
    "*Template*",
]

# Known ARMCC installation paths to search
_KNOWN_ARMCC_PATHS = [
    "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin",
    "C:/Keil_v5/ARM/ARMCC/bin",
    "C:/Keil/ARM/ARMCC/bin",
]


class ProjectDetector:
    """Auto-detects embedded project configuration from directory structure."""

    def __init__(
        self,
        project_dir: str,
        chip_db: Optional[ChipDB] = None,
        toolchain_registry: Optional[ToolchainRegistry] = None,
    ):
        self._project_dir = os.path.abspath(project_dir)
        self._chip_db = chip_db or ChipDB()
        self._toolchain_registry = toolchain_registry or ToolchainRegistry()

        # Ensure databases are loaded
        if len(self._chip_db) == 0:
            self._chip_db.load_builtin()
        if len(self._toolchain_registry) == 0:
            self._toolchain_registry.load_builtin()

    def detect(self) -> ProjectConfig:
        """Run full auto-detection and return a ProjectConfig.

        Returns:
            ProjectConfig with all detected values filled in.
            Fields that couldn't be detected remain at defaults.
        """
        config = ProjectConfig(project_dir=self._project_dir)

        # Pass 1: CubeMX project
        self._detect_cubemx(config)

        # Pass 2: Chip info
        self._detect_chip(config)

        # Pass 3: RTOS
        self._detect_rtos(config)

        # Pass 4: Toolchain
        self._detect_toolchains(config)

        # Pass 5: Startup and linker files
        self._detect_startup_file(config)
        self._detect_linker_script(config)

        # Pass 6: Project name fallback
        self._detect_project_name(config)

        # Build default scan config
        self._build_scan_config(config)

        return config

    def _detect_cubemx(self, config: ProjectConfig) -> None:
        """Detect CubeMX project from .ioc file."""
        ioc_file = ioc_parser.find_ioc_file(self._project_dir)
        if not ioc_file:
            return

        config.is_cubemx_project = True
        config.ioc_file = ioc_file
        data = ioc_parser.parse_ioc(ioc_file)

        chip_model = ioc_parser.extract_chip_model(data)
        if chip_model:
            config.chip_model = chip_model

        chip_family = ioc_parser.extract_chip_family(data)
        if chip_family:
            config.chip_family = chip_family

        proj_name = ioc_parser.extract_project_name(data)
        if proj_name:
            config.project_name = proj_name

        # Look up full chip info
        if config.chip_family:
            chip_info = self._chip_db.find_family(config.chip_family)
            if not chip_info and config.chip_model:
                chip_info = self._chip_db.find_family_for_model(config.chip_model)
            config.chip_info = chip_info
            if chip_info:
                config.defines = list(chip_info.default_defines)
                if config.chip_model:
                    model = _resolve_chip_model(chip_info, config.chip_model)
                    if model:
                        config.chip_model = model.name  # normalize name
                        config.defines.extend(model.defines)

        # RTOS detection from .ioc
        rtos_type = ioc_parser.has_rtos(data)
        if rtos_type:
            config.rtos.type = rtos_type

        # Toolchain preference from .ioc
        tc = ioc_parser.extract_toolchain(data)
        if tc and tc not in config.toolchains:
            config.toolchains.append(tc)

        # Defines from .ioc
        ioc_defines = ioc_parser.extract_defines(data)
        for d in ioc_defines:
            if d not in config.defines:
                config.defines.append(d)

    def _detect_chip(self, config: ProjectConfig) -> None:
        """Detect chip from headers if not found via .ioc."""
        if config.chip_info and config.chip_model:
            return  # Already detected from .ioc

        # Search for chip headers in common locations
        search_dirs = [
            os.path.join(self._project_dir, "Core", "Inc"),
            os.path.join(self._project_dir, "Inc"),
            self._project_dir,
        ]

        for search_dir in search_dirs:
            if not os.path.isdir(search_dir):
                continue
            for f in os.listdir(search_dir):
                for pattern, family_fmt in _CHIP_HEADER_PATTERNS:
                    import re
                    m = re.match(pattern, f, re.IGNORECASE)
                    if m:
                        # Try to get the full family name
                        group_count = len(m.groups())
                        if group_count >= 1:
                            args = [m.group(i + 1) for i in range(group_count)]
                            family = family_fmt.format(*args)
                        else:
                            family = family_fmt

                        chip_info = self._chip_db.find_family(family)
                        if chip_info:
                            config.chip_info = chip_info
                            config.chip_family = family
                            config.defines = list(chip_info.default_defines)
                            return

    def _detect_rtos(self, config: ProjectConfig) -> None:
        """Detect RTOS type and configuration."""
        # Already detected from .ioc
        if config.rtos.type and config.rtos.type != "none":
            return

        # Search for FreeRTOSConfig.h
        for root, dirs, files in os.walk(self._project_dir):
            if ".git" in dirs:
                dirs.remove(".git")
            for f in files:
                if f == "FreeRTOSConfig.h":
                    config.rtos.type = "FreeRTOS"
                    break
            if config.rtos.type == "FreeRTOS":
                break

        if config.rtos.type == "FreeRTOS":
            self._detect_freertos_details(config)

    def _detect_freertos_details(self, config: ProjectConfig) -> None:
        """Detect FreeRTOS port paths and heap configuration."""
        # Find the FreeRTOS root directory
        rtos_root = self._find_dir(self._project_dir, "FreeRTOS")
        if not rtos_root:
            return

        config.rtos.base_path = rtos_root

        # Detect available heap implementations
        mem_mang_dir = os.path.join(rtos_root, "Source", "portable", "MemMang")
        if os.path.isdir(mem_mang_dir):
            # Prefer heap_4.c
            heap4 = os.path.join(mem_mang_dir, "heap_4.c")
            if os.path.isfile(heap4):
                config.rtos.heap = "heap_4.c"

        # Detect CMSIS-RTOS version
        cmsis_v2 = os.path.join(rtos_root, "Source", "CMSIS_RTOS_V2")
        if os.path.isdir(cmsis_v2):
            config.rtos.cmsis_version = "V2"
        else:
            cmsis_v1 = os.path.join(rtos_root, "Source", "CMSIS_RTOS")
            if os.path.isdir(cmsis_v1):
                config.rtos.cmsis_version = "V1"

        # Build port mapping for each toolchain
        portable_dir = os.path.join(rtos_root, "Source", "portable")
        for tc_id, port_dir in _FREERTOS_PORT_DIRS.items():
            full_port_dir = os.path.join(portable_dir, port_dir)
            if os.path.isdir(full_port_dir):
                # Find the best CPU architecture subdirectory
                subdirs = [d for d in os.listdir(full_port_dir)
                           if os.path.isdir(os.path.join(full_port_dir, d))]
                if subdirs:
                    # Prefer matching CPU arch if we know it
                    config.rtos.port_mapping[tc_id] = f"{port_dir}/{subdirs[0]}"

    def _detect_toolchains(self, config: ProjectConfig) -> None:
        """Detect available toolchains."""
        if not config.toolchains:
            config.toolchains = []

        # GCC is always available if prefix is on PATH
        if "gcc" not in config.toolchains:
            if _which("arm-none-eabi-gcc"):
                config.toolchains.append("gcc")

        # Check ARMCC
        if "armcc" not in config.toolchains:
            if _find_armcc():
                config.toolchains.append("armcc")
            if not config.armcc_compiler_dir:
                armcc_dir = _find_armcc()
                if armcc_dir:
                    config.armcc_compiler_dir = armcc_dir

        # Default to gcc if nothing found
        if not config.toolchains:
            config.toolchains = ["gcc"]

    def _detect_startup_file(self, config: ProjectConfig) -> None:
        """Detect startup file paths."""
        chip_info = config.chip_info

        # GCC startup file in project root
        if not config.startup_file and chip_info and chip_info.startup.pattern:
            pattern = os.path.join(self._project_dir, chip_info.startup.pattern)
            matches = glob.glob(pattern)
            if matches:
                config.startup_file = os.path.basename(matches[0])
            elif chip_info.startup.gcc_default:
                default_path = os.path.join(self._project_dir, chip_info.startup.gcc_default)
                if os.path.isfile(default_path):
                    config.startup_file = chip_info.startup.gcc_default

        # Legacy pattern: search for any startup_stm32* in root
        if not config.startup_file:
            for f in os.listdir(self._project_dir):
                if f.startswith("startup_") and f.endswith((".s", ".S")):
                    config.startup_file = f
                    break

        # ARMCC startup file
        if not config.armcc_startup_file and chip_info and chip_info.startup.armcc_default:
            full_path = os.path.join(self._project_dir, chip_info.startup.armcc_default)
            if os.path.isfile(full_path):
                config.armcc_startup_file = chip_info.startup.armcc_default

        # Legacy: check MDK-ARM Templates directory
        if not config.armcc_startup_file and chip_info and chip_info.startup.armcc_pattern:
            pattern = os.path.join(self._project_dir, chip_info.startup.armcc_pattern)
            matches = glob.glob(pattern)
            if matches:
                config.armcc_startup_file = os.path.relpath(matches[0], self._project_dir)

    def _detect_linker_script(self, config: ProjectConfig) -> None:
        """Detect linker script and scatter file paths."""
        chip_info = config.chip_info

        # GCC linker script
        if not config.linker_script:
            if chip_info and chip_info.linker.gcc_pattern:
                pattern = os.path.join(self._project_dir, chip_info.linker.gcc_pattern)
                matches = glob.glob(pattern)
                if matches:
                    config.linker_script = os.path.basename(matches[0])

            # Legacy: search for *FLASH*.ld
            if not config.linker_script:
                for f in glob.glob(os.path.join(self._project_dir, "*FLASH*.ld")):
                    config.linker_script = os.path.basename(f)
                    break

        # ARMCC scatter file
        if not config.armcc_scatter_file:
            if chip_info and chip_info.linker.armcc_pattern:
                pattern = os.path.join(self._project_dir, "MDK-ARM", "**", chip_info.linker.armcc_pattern)
                matches = glob.glob(pattern, recursive=True)
                if matches:
                    config.armcc_scatter_file = os.path.relpath(matches[0], self._project_dir).replace("\\", "/")

            # Legacy: check MDK-ARM/Project/
            if not config.armcc_scatter_file:
                for p in glob.glob(os.path.join(self._project_dir, "MDK-ARM", "**", "*.sct"), recursive=True):
                    config.armcc_scatter_file = os.path.relpath(p, self._project_dir).replace("\\", "/")
                    break

    def _detect_project_name(self, config: ProjectConfig) -> None:
        """Detect project name from various sources."""
        if config.project_name:
            return

        # Try Makefile
        mk_file = makefile_parser.find_makefile(self._project_dir)
        if mk_file:
            mk_data = makefile_parser.parse_makefile(mk_file)
            name = makefile_parser.extract_target(mk_data)
            if name:
                config.project_name = name
                return

        # Try CMakeLists.txt
        cmake_file = os.path.join(self._project_dir, "CMakeLists.txt")
        if os.path.isfile(cmake_file):
            name = _extract_cmake_project(cmake_file)
            if name:
                config.project_name = name
                return

        # Fallback: directory name
        config.project_name = os.path.basename(self._project_dir)

    def _build_scan_config(self, config: ProjectConfig) -> None:
        """Build default scan configuration for the detected project."""
        scan = config.scan

        # Default exclude dirs for embedded projects
        default_exclude = set(_DEFAULT_EXCLUDE_DIRS)
        for d in default_exclude:
            if d not in scan.exclude_dirs:
                scan.exclude_dirs.append(d)

        # Default exclude dir patterns
        if "build*" not in scan.exclude_dir_patterns:
            scan.exclude_dir_patterns.append("build*")

        # Default exclude file patterns
        for p in _DEFAULT_EXCLUDE_FILE_PATTERNS:
            if p not in scan.exclude_file_patterns:
                scan.exclude_file_patterns.append(p)

        # Default exclude files (always)
        for f in _DEFAULT_EXCLUDE_FILES:
            if f not in scan.exclude_files:
                scan.exclude_files.append(f)

        # CMSIS-RTOS V1 always excluded (use V2)
        if "cmsis_os.c" not in scan.exclude_files:
            scan.exclude_files.append("cmsis_os.c")
        if "cmsis_os1.c" not in scan.exclude_files:
            scan.exclude_files.append("cmsis_os1.c")
        if "CMSIS_RTOS" not in scan.exclude_dirs:
            scan.exclude_dirs.append("CMSIS_RTOS")

        # RTOS-specific excludes
        if config.rtos.type == "FreeRTOS":
            # Exclude non-selected heap implementations
            heap_name = config.rtos.heap
            for i in range(1, 6):
                h = f"heap_{i}.c"
                if h != heap_name and h not in scan.exclude_files:
                    scan.exclude_files.append(h)

            # Exclude CMSIS-RTOS V1 if V2 is used
            if config.rtos.cmsis_version == "V2":
                if "cmsis_os.c" not in scan.exclude_files:
                    scan.exclude_files.append("cmsis_os.c")
                if "cmsis_os1.c" not in scan.exclude_files:
                    scan.exclude_files.append("cmsis_os1.c")
                if _CMSIS_RTOS_V1_DIR not in scan.exclude_dirs:
                    scan.exclude_dirs.append(_CMSIS_RTOS_V1_DIR)

        # ARMCC port dirs (excluded from base scan, added conditionally)
        if "RVDS" not in scan.exclude_dirs:
            scan.exclude_dirs.append("RVDS")

    @staticmethod
    def _find_dir(base: str, name: str) -> Optional[str]:
        """Find a directory by name recursively under base."""
        for root, dirs, files in os.walk(base):
            if ".git" in dirs:
                dirs.remove(".git")
            for d in dirs:
                if d == name:
                    return os.path.join(root, d)
        return None


def _which(cmd: str) -> Optional[str]:
    """Find an executable on PATH. Cross-platform wrapper."""
    for path_dir in os.environ.get("PATH", "").split(os.pathsep):
        ext = ".exe" if os.name == "nt" else ""
        full_path = os.path.join(path_dir, cmd + ext)
        if os.path.isfile(full_path):
            return full_path
    return None


def _find_armcc() -> Optional[str]:
    """Find ARMCC compiler installation directory."""
    for p in _KNOWN_ARMCC_PATHS:
        armcc = os.path.join(p, "armcc.exe")
        if os.path.isfile(armcc):
            return p
    return None


def _extract_cmake_project(filepath: str) -> Optional[str]:
    """Extract project name from CMakeLists.txt 'project(...)' call."""
    import re
    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()
    m = re.search(r"project\s*\(\s*(\w+)", content)
    if m:
        return m.group(1)
    return None


def _resolve_chip_model(chip_info: 'ChipInfo', model_name: str) -> Optional['ChipModel']:
    """Resolve chip model with fuzzy matching.

    Handles CubeMX naming: STM32F103C8T6 -> STM32F103C8Tx in DB.
    """
    import re
    # Exact match
    model = chip_info.get_model(model_name)
    if model:
        return model

    # Fuzzy: normalize to Tx suffix
    normalized = re.sub(r'(\d)[A-Z]\d$', r'\1Tx', model_name)
    if normalized != model_name:
        return chip_info.get_model(normalized)

    return None

