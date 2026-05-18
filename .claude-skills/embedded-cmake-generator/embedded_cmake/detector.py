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
import json
import os
import re
from pathlib import Path
from typing import Any, Dict, List, Optional

from .chip_db import ChipDB, resolve_chip_model
from .models import ProjectConfig, ScanConfig
from .toolchain import ToolchainRegistry
from .parsers import ioc_parser, makefile_parser, rtthread_parser, uvprojx_parser
from .utils import detect_monolithic_includes, read_file_dual_encoding


# ---------------------------------------------------------------------------
# JSON data loading helpers
# ---------------------------------------------------------------------------


def _data_dir() -> str:
    """Get the data directory path."""
    return os.path.join(os.path.dirname(__file__), "data")


def _load_json_data(filename: str) -> Dict[str, Any]:
    """Load a JSON file from the data directory.

    Returns empty dict on any load failure (missing file, parse error).
    """
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


def _load_scan_defaults() -> Dict[str, Any]:
    """Load scan default configuration from data/scan_defaults.json."""
    return _load_json_data("scan_defaults.json")


# ---------------------------------------------------------------------------
# Chip header detection patterns (loaded from external JSON)
# ---------------------------------------------------------------------------


def _default_patterns_dir() -> str:
    """Get the default data directory."""
    return _data_dir()


_HEADER_PATTERNS_FILENAME = "_header_patterns.json"

# Cache for loaded patterns (module-level singleton)
_loaded_header_patterns: Optional[List[tuple]] = None
_loaded_user_patterns: Dict[str, Dict[str, str]] = {}


def _load_header_patterns(
    extra_patterns: Optional[List[Dict[str, str]]] = None,
) -> List[tuple]:
    """Load chip header patterns from JSON, falling back to hardcoded defaults.

    Args:
        extra_patterns: Optional list of user-defined patterns from config.
            Each dict: {"regex": "...", "family": "..."}

    Returns:
        List of (compiled_regex, family_template) tuples.
    """
    global _loaded_header_patterns

    if _loaded_header_patterns is None:
        builtin: List[tuple] = []
        json_path = os.path.join(_default_patterns_dir(), _HEADER_PATTERNS_FILENAME)
        if os.path.isfile(json_path):
            try:
                with open(json_path, "r", encoding="utf-8") as f:
                    data = json.load(f)
                for entry in data:
                    builtin.append((re.compile(entry["regex"], re.IGNORECASE),
                                    entry["family"]))
            except (OSError, json.JSONDecodeError, KeyError):
                builtin = []  # fall through to hardcoded below
        if not builtin:
            builtin = _HARDCODED_HEADER_PATTERNS
        _loaded_header_patterns = builtin

    result = list(_loaded_header_patterns)
    if extra_patterns:
        for ep in extra_patterns:
            result.append((re.compile(ep["regex"], re.IGNORECASE), ep["family"]))
    return result


_HARDCODED_HEADER_PATTERNS: list = []  # JSON fallback only (data/_header_patterns.json is authoritative)

_KNOWN_ARMCC_PATHS = [
    "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin",
    "C:/Keil_v5/ARM/ARMCC/bin",
    "C:/Keil/ARM/ARMCC/bin",
]

# Environment variable to override ARMCC path
_ARMCC_ENV_VAR = "ARMCC_PATH"
_KEIL_ROOT_ENV_VAR = "KEIL_ROOT"


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

        if len(self._chip_db) == 0:
            self._chip_db.load_builtin()
        if len(self._toolchain_registry) == 0:
            self._toolchain_registry.load_builtin()

    def detect(self) -> ProjectConfig:
        """Run full auto-detection and return a ProjectConfig."""
        config = ProjectConfig(project_dir=self._project_dir)

        ioc_file = ioc_parser.find_ioc_file(self._project_dir)

        # Always try Keil detection — a project can have both .ioc and .uvprojx
        self._detect_keil(config)
        if ioc_file:
            self._detect_cubemx(config)
        elif not config.is_keil_project:
            self._detect_cubemx(config)

        if not config.chip_info:
            self._detect_chip(config)
        if not config.chip_info:
            self._detect_from_makefile(config)
        self._detect_rtos(config)
        self._detect_toolchains(config)
        self._detect_startup_file(config)
        self._detect_linker_script(config)

        if config.is_keil_project:
            self._detect_monolithic(config)

        self._detect_project_name(config)
        self._build_scan_config(config)

        return config

    # -- detection passes --------------------------------------------------

    def _detect_keil(self, config: ProjectConfig) -> None:
        """Detect Keil MDK project from .uvprojx file."""
        uvprojx_file = uvprojx_parser.find_uvprojx_file(self._project_dir)
        if not uvprojx_file:
            return

        config.is_keil_project = True
        config.keil_project_file = uvprojx_file

        try:
            data = uvprojx_parser.parse_uvprojx(uvprojx_file, self._project_dir)
        except Exception:
            config.is_keil_project = False
            config.keil_project_file = ""
            return

        device, vendor, cpu_type = uvprojx_parser.extract_device_info(data)
        if device:
            config.chip_model = device
            if vendor:
                family = _infer_family_from_device(device, vendor)
                chip_info = self._chip_db.find_family(family)
                if not chip_info:
                    chip_info = self._chip_db.find_family_for_model(device)
                config.chip_info = chip_info
                if chip_info:
                    config.chip_family = chip_info.family
                    config.defines = list(chip_info.default_defines)
                    if config.chip_model:
                        model = resolve_chip_model(chip_info, config.chip_model)
                        if model:
                            config.chip_model = model.name
                            config.defines.extend(model.defines)

        config.cpu_name = cpu_type

        for d in uvprojx_parser.extract_defines(data):
            if d not in config.defines:
                config.defines.append(d)
        config.keil_include_paths = uvprojx_parser.extract_include_paths(data)
        config.keil_source_groups = data.get("groups", [])
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

        config.lib_files = uvprojx_parser.extract_lib_files(data)

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

        if config.chip_family:
            chip_info = self._chip_db.find_family(config.chip_family)
            if not chip_info and config.chip_model:
                chip_info = self._chip_db.find_family_for_model(config.chip_model)
            config.chip_info = chip_info
            if chip_info:
                config.defines = list(chip_info.default_defines)
                if config.chip_model:
                    model = resolve_chip_model(chip_info, config.chip_model)
                    if model:
                        config.chip_model = model.name
                        config.defines.extend(model.defines)

        rtos_type = ioc_parser.has_rtos(data)
        if rtos_type:
            config.rtos.type = rtos_type
        tc = ioc_parser.extract_toolchain(data)
        if tc and tc not in config.toolchains:
            config.toolchains.append(tc)
        for d in ioc_parser.extract_defines(data):
            if d not in config.defines:
                config.defines.append(d)

        self._detect_cubemx_drivers(config)

    def _detect_cubemx_drivers(self, config: ProjectConfig) -> None:
        """Add HAL/CMSIS include paths and source files from CubeMX repo root.

        Walks up from the .ioc file to find the ``Drivers/`` directory
        (standard CubeMX layout), then adds the relevant HAL driver
        include paths and source files to the config.
        """
        if not config.ioc_file:
            return

        repo_root = ioc_parser.find_cubemx_repo_root(config.ioc_file)
        if not repo_root:
            return

        chip_info = config.chip_info
        if not chip_info:
            return

        hal_name = chip_info.hal_driver_name
        if hal_name:
            # HAL driver include dir
            hal_inc = os.path.join(repo_root, "Drivers", hal_name, "Inc")
            if os.path.isdir(hal_inc) and hal_inc not in config.cubemx_extra_includes:
                config.cubemx_extra_includes.append(hal_inc)

            # HAL driver source files (scan only the HAL Src/ dir, not the whole repo)
            hal_src = os.path.join(repo_root, "Drivers", hal_name, "Src")
            if os.path.isdir(hal_src):
                for f in sorted(os.listdir(hal_src)):
                    if not f.endswith(".c"):
                        continue
                    if "template" in f.lower():
                        continue
                    src_path = os.path.join(hal_src, f)
                    if src_path not in config.cubemx_extra_sources:
                        config.cubemx_extra_sources.append(src_path)

            # CMSIS include dir
            cmsis_inc = os.path.join(repo_root, "Drivers", "CMSIS", "Include")
            if os.path.isdir(cmsis_inc) and cmsis_inc not in config.cubemx_extra_includes:
                config.cubemx_extra_includes.append(cmsis_inc)

            # CMSIS Device include dir (e.g. Drivers/CMSIS/Device/ST/STM32G0xx/Include)
            family = chip_info.family  # e.g. "STM32G0"
            family_xx = f"{family}xx"
            device_inc = os.path.join(
                repo_root, "Drivers", "CMSIS", "Device", "ST", family_xx, "Include"
            )
            if os.path.isdir(device_inc) and device_inc not in config.cubemx_extra_includes:
                config.cubemx_extra_includes.append(device_inc)

    def _detect_chip(self, config: ProjectConfig) -> None:
        """Detect chip from headers if not found via .ioc."""
        if config.chip_info and config.chip_model:
            return

        search_dirs = [
            os.path.join(self._project_dir, "Core", "Inc"),
            os.path.join(self._project_dir, "Inc"),
            self._project_dir,
        ]

        patterns = _load_header_patterns(config.extra_header_patterns)

        for search_dir in search_dirs:
            if not os.path.isdir(search_dir):
                continue
            for f in os.listdir(search_dir):
                for pattern, family_fmt in patterns:
                    m = pattern.match(f)
                    if m:
                        group_count = len(m.groups())
                        args = [m.group(i + 1) for i in range(group_count)] if group_count >= 1 else []
                        family = family_fmt.format(*args) if args else family_fmt

                        chip_info = self._chip_db.find_family(family)
                        if chip_info:
                            config.chip_info = chip_info
                            config.chip_family = family
                            config.defines = list(chip_info.default_defines)
                            return

    def _detect_rtos(self, config: ProjectConfig) -> None:
        """Detect RTOS type and configuration."""
        if config.rtos.type and config.rtos.type != "none":
            if config.rtos.type == "FreeRTOS":
                self._detect_freertos_details(config)
            elif config.rtos.type == "uCOS-II":
                self._detect_ucos_details(config)
            return

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
            return

        if not config.rtos.type or config.rtos.type == "none":
            for root, dirs, files in os.walk(self._project_dir):
                if ".git" in dirs:
                    dirs.remove(".git")
                for f in files:
                    if f == "ucos_ii.h":
                        config.rtos.type = "uCOS-II"
                        break
                if config.rtos.type == "uCOS-II":
                    break

        if config.rtos.type == "uCOS-II":
            self._detect_ucos_details(config)

        # RT-Thread detection
        if not config.rtos.type or config.rtos.type == "none":
            for root, dirs, files in os.walk(self._project_dir):
                if ".git" in dirs:
                    dirs.remove(".git")
                for f in files:
                    if f == "rtconfig.h":
                        config.rtos.type = "RT-Thread"
                        break
                if config.rtos.type == "RT-Thread":
                    break

        if config.rtos.type == "RT-Thread":
            self._detect_rtthread_details(config)

    def _detect_toolchains(self, config: ProjectConfig) -> None:
        """Detect available toolchains."""
        if not config.toolchains:
            config.toolchains = []

        if "gcc" not in config.toolchains and _which("arm-none-eabi-gcc"):
            config.toolchains.append("gcc")

        if "armcc" not in config.toolchains:
            armcc_dir = _find_armcc()
            if armcc_dir:
                config.toolchains.append("armcc")
                if not config.armcc_compiler_dir:
                    config.armcc_compiler_dir = armcc_dir

        if not config.toolchains:
            config.toolchains = ["gcc"]

    def _detect_startup_file(self, config: ProjectConfig) -> None:
        """Detect startup file paths."""
        chip_info = config.chip_info

        if not config.startup_file and chip_info and chip_info.startup.pattern:
            matches = glob.glob(os.path.join(self._project_dir, chip_info.startup.pattern))
            if matches:
                config.startup_file = os.path.basename(matches[0])
            elif chip_info.startup.gcc_default:
                default_path = os.path.join(self._project_dir, chip_info.startup.gcc_default)
                if os.path.isfile(default_path):
                    config.startup_file = chip_info.startup.gcc_default

        if not config.startup_file:
            for f in os.listdir(self._project_dir):
                if f.startswith("startup_") and f.endswith((".s", ".S")):
                    config.startup_file = f
                    break

        if not config.armcc_startup_file and chip_info and chip_info.startup.armcc_default:
            full_path = os.path.join(self._project_dir, chip_info.startup.armcc_default)
            if os.path.isfile(full_path):
                config.armcc_startup_file = chip_info.startup.armcc_default

        if not config.armcc_startup_file and chip_info and chip_info.startup.armcc_pattern:
            matches = glob.glob(os.path.join(self._project_dir, chip_info.startup.armcc_pattern))
            if matches:
                config.armcc_startup_file = os.path.relpath(matches[0], self._project_dir)

    def _detect_linker_script(self, config: ProjectConfig) -> None:
        """Detect linker script and scatter file paths."""
        chip_info = config.chip_info

        if not config.linker_script:
            if chip_info and chip_info.linker.gcc_pattern:
                matches = glob.glob(os.path.join(self._project_dir, chip_info.linker.gcc_pattern))
                if matches:
                    config.linker_script = os.path.basename(matches[0])
            if not config.linker_script:
                for f in glob.glob(os.path.join(self._project_dir, "*FLASH*.ld")):
                    config.linker_script = os.path.basename(f)
                    break

        if not config.armcc_scatter_file:
            if chip_info and chip_info.linker.armcc_pattern:
                matches = glob.glob(os.path.join(self._project_dir, "MDK-ARM", "**", chip_info.linker.armcc_pattern), recursive=True)
                if matches:
                    config.armcc_scatter_file = os.path.relpath(matches[0], self._project_dir).replace("\\", "/")
            if not config.armcc_scatter_file:
                for pattern in ["MDK-ARM/**/*.sct", "KeilPrj/**/*.sct",
                                "source/Hardware/**/*.sct", "**/*.sct"]:
                    for p in glob.glob(os.path.join(self._project_dir, pattern), recursive=True):
                        config.armcc_scatter_file = os.path.relpath(p, self._project_dir).replace("\\", "/")
                        break
                    if config.armcc_scatter_file:
                        break

    def _detect_project_name(self, config: ProjectConfig) -> None:
        """Detect project name from various sources."""
        if config.project_name:
            return

        mk_file = makefile_parser.find_makefile(self._project_dir)
        if mk_file:
            mk_data = makefile_parser.parse_makefile(mk_file)
            name = makefile_parser.extract_target(mk_data)
            if name:
                config.project_name = name
                return

        cmake_file = os.path.join(self._project_dir, "CMakeLists.txt")
        if os.path.isfile(cmake_file):
            with open(cmake_file, "r", encoding="utf-8", errors="ignore") as f:
                content = f.read()
            m = re.search(r"project\s*\(\s*(\w+)", content)
            if m:
                config.project_name = m.group(1)
                return

        config.project_name = os.path.basename(self._project_dir)

    def _detect_freertos_details(self, config: ProjectConfig) -> None:
        """Detect FreeRTOS port paths and heap configuration."""
        rtos_root = ProjectDetector._find_dir(self._project_dir, "FreeRTOS")
        if not rtos_root:
            return
        config.rtos.base_path = rtos_root

        mem_mang_dir = os.path.join(rtos_root, "Source", "portable", "MemMang")
        if os.path.isdir(mem_mang_dir):
            heap4 = os.path.join(mem_mang_dir, "heap_4.c")
            if os.path.isfile(heap4):
                config.rtos.heap = "heap_4.c"

        cmsis_v2 = os.path.join(rtos_root, "Source", "CMSIS_RTOS_V2")
        config.rtos.cmsis_version = "V2" if os.path.isdir(cmsis_v2) else ""

        portable_dir = os.path.join(rtos_root, "Source", "portable")
        for tc_id, port_dir in _load_rtos_config().get("freertos", {}).get("port_subdirs", {}).items():
            full_port_dir = os.path.join(portable_dir, port_dir)
            if os.path.isdir(full_port_dir):
                subdirs = [d for d in os.listdir(full_port_dir)
                           if os.path.isdir(os.path.join(full_port_dir, d))]
                if subdirs:
                    config.rtos.port_mapping[tc_id] = f"{port_dir}/{subdirs[0]}"

    def _detect_ucos_details(self, config: ProjectConfig) -> None:
        """Detect uCOS-II port paths and configuration."""
        rtos_root = ProjectDetector._find_dir(self._project_dir, "uCOSII")
        if not rtos_root:
            return
        config.rtos.base_path = rtos_root

        port_base = os.path.join(rtos_root, "uC-PORT")
        if os.path.isdir(port_base):
            for entry in os.listdir(port_base):
                port_dir_entry = os.path.join(port_base, entry)
                if not os.path.isdir(port_dir_entry):
                    continue
                for tc_dir_name, tc_id in [
                    ("RealView", "armcc"),
                    ("GNU", "gcc"),
                    ("IAR", "iar"),
                ]:
                    tc_path = os.path.join(port_dir_entry, "Generic", tc_dir_name)
                    if os.path.isdir(tc_path):
                        rel = os.path.relpath(tc_path, self._project_dir)
                        if tc_id not in config.rtos.port_mapping:
                            config.rtos.port_mapping[tc_id] = rel

    def _detect_rtthread_details(self, config: ProjectConfig) -> None:
        """Detect RT-Thread configuration from ``rtconfig.h``."""
        for root, dirs, files in os.walk(self._project_dir):
            if ".git" in dirs:
                dirs.remove(".git")
            for f in files:
                if f == "rtconfig.h":
                    rtconfig_path = os.path.join(root, f)
                    try:
                        defines = rtthread_parser.parse_rtconfig(rtconfig_path)
                        for d in defines:
                            if d not in config.defines:
                                config.defines.append(d)
                    except Exception:
                        pass
                    return

    def _detect_from_makefile(self, config: ProjectConfig) -> None:
        """Detect chip from Makefile defines and CPU flags.

        Last-resort detection when no ``.ioc``, ``.uvprojx``, or chip header
        is available.  Uses ``-D`` defines and ``-mcpu`` flags extracted from
        the Makefile.
        """
        mk_file = makefile_parser.find_makefile(self._project_dir)
        if not mk_file:
            return

        mk_data = makefile_parser.parse_makefile(mk_file)
        defines = makefile_parser.extract_defines(mk_data)
        cpu_flags = makefile_parser.extract_cpu_flags(mk_data)

        # Try to find chip via define -> family index
        for d in defines:
            name = d.split("=")[0] if "=" in d else d
            chip_info = self._chip_db.find_family_by_define(name)
            if chip_info:
                config.chip_info = chip_info
                config.chip_family = chip_info.family
                config.defines.extend(chip_info.default_defines)
                return

        # Fallback: try matching defines as chip model names
        for d in defines:
            name = d.split("=")[0] if "=" in d else d
            chip_info = self._chip_db.find_family_for_model(name)
            if chip_info:
                config.chip_info = chip_info
                config.chip_family = chip_info.family
                config.defines.extend(chip_info.default_defines)
                return

        # Fallback: match -mcpu to core architecture
        cpu = cpu_flags.get("cpu", "")
        if cpu:
            core_map = {
                "cortex-m0": "Cortex-M0",
                "cortex-m0+": "Cortex-M0+",
                "cortex-m0plus": "Cortex-M0+",
                "cortex-m3": "Cortex-M3",
                "cortex-m4": "Cortex-M4",
                "cortex-m7": "Cortex-M7",
            }
            arch_name = core_map.get(cpu)
            if arch_name:
                for family_name in self._chip_db.list_ids():
                    chip_info = self._chip_db.find_family(family_name)
                    if chip_info and chip_info.cpu.core == arch_name:
                        config.chip_info = chip_info
                        config.chip_family = chip_info.family
                        config.defines.extend(chip_info.default_defines)
                        return

    def _detect_monolithic(self, config: ProjectConfig) -> None:
        """Detect monolithic .c includes if not already detected from Keil."""
        if config.monolithic.leaf_files or config.monolithic.root_files:
            return

        c_files = []
        for root, dirs, files in os.walk(self._project_dir):
            dirs[:] = [d for d in dirs if d not in (".git", "build", ".vscode")]
            for f in files:
                if f.endswith(".c"):
                    c_files.append(os.path.join(root, f))

        if c_files:
            config.monolithic = detect_monolithic_includes(c_files[:200], self._project_dir)

    def _build_scan_config(self, config: ProjectConfig) -> None:
        """Build default scan configuration for the detected project."""
        scan = config.scan

        scan_defaults = _load_scan_defaults()
        for d in scan_defaults.get("exclude_dirs", []):
            if d not in scan.exclude_dirs:
                scan.exclude_dirs.append(d)
        if "build*" not in scan.exclude_dir_patterns:
            scan.exclude_dir_patterns.append("build*")
        for p in scan_defaults.get("exclude_file_patterns", []):
            if p not in scan.exclude_file_patterns:
                scan.exclude_file_patterns.append(p)
        for f in scan_defaults.get("exclude_files", []):
            if f not in scan.exclude_files:
                scan.exclude_files.append(f)

        for exclude in ["cmsis_os.c", "cmsis_os1.c"]:
            if exclude not in scan.exclude_files:
                scan.exclude_files.append(exclude)
        if "CMSIS_RTOS" not in scan.exclude_dirs:
            scan.exclude_dirs.append("CMSIS_RTOS")

        if config.rtos.type == "FreeRTOS":
            heap_name = config.rtos.heap
            for i in range(1, 6):
                h = f"heap_{i}.c"
                if h != heap_name and h not in scan.exclude_files:
                    scan.exclude_files.append(h)
            if config.rtos.cmsis_version == "V2":
                if "cmsis_os.c" not in scan.exclude_files:
                    scan.exclude_files.append("cmsis_os.c")
                if "cmsis_os1.c" not in scan.exclude_files:
                    scan.exclude_files.append("cmsis_os1.c")
                cmsis_v1 = _load_rtos_config().get("freertos", {}).get("cmsis_v1_dir", "CMSIS_RTOS")
                if cmsis_v1 not in scan.exclude_dirs:
                    scan.exclude_dirs.append(cmsis_v1)

        if "RVDS" not in scan.exclude_dirs:
            scan.exclude_dirs.append("RVDS")

    # -- static helpers ----------------------------------------------------

    @staticmethod
    def _find_dir(base: str, name: str) -> Optional[str]:
        """Find a directory by name recursively under base.

        Skips CMake build directories and common build output dirs.
        """
        exclude_names = {"build", "cmake-build", "Debug", "Release"}
        for root, dirs, files in os.walk(base):
            if ".git" in dirs:
                dirs.remove(".git")
            # Exclude CMake build dirs and common build output dirs
            dirs[:] = [d for d in dirs if d not in exclude_names]
            dirs[:] = [d for d in dirs
                       if not os.path.isfile(os.path.join(root, d, "CMakeCache.txt"))]
            for d in dirs:
                if d == name:
                    return os.path.join(root, d)
        return None


# ---------------------------------------------------------------------------
# Module-level helpers (infer_family, which, find_armcc)
# ---------------------------------------------------------------------------

def _infer_family_from_device(device: str, vendor: str) -> str:
    """Infer chip family name from device name and vendor."""
    if "APM32" in device.upper():
        m = re.match(r"APM32([A-Z]\d+)", device, re.IGNORECASE)
        return f"APM32{m.group(1)}" if m else "APM32E1"

    m = re.match(r"STM32([A-Z]\d+)", device, re.IGNORECASE)
    if m:
        return f"STM32{m.group(1)}"

    m = re.match(r"MK(\d+)", device, re.IGNORECASE)
    if m:
        return f"MK{m.group(1)}"

    return device


def _which(cmd: str) -> Optional[str]:
    """Find an executable on PATH. Cross-platform wrapper."""
    for path_dir in os.environ.get("PATH", "").split(os.pathsep):
        ext = ".exe" if os.name == "nt" else ""
        full_path = os.path.join(path_dir, cmd + ext)
        if os.path.isfile(full_path):
            return full_path
    return None


def _find_armcc() -> Optional[str]:
    """Find ARMCC compiler installation directory.

    Priority:
    1. ARMCC_PATH or KEIL_ROOT environment variables
    2. Windows registry (HKLM\\SOFTWARE\\Keil\\MDK, Windows only)
    3. Known hardcoded paths (fallback)
    """
    env_path = os.environ.get(_ARMCC_ENV_VAR) or os.environ.get(_KEIL_ROOT_ENV_VAR)
    if env_path:
        bin_dir = os.path.join(env_path, "ARM", "ARMCC", "bin") if "ARMCC" not in env_path else env_path
        if os.path.isfile(os.path.join(bin_dir, "armcc.exe")):
            return bin_dir
        if os.path.isfile(os.path.join(env_path, "armcc.exe")):
            return env_path

    # Windows registry (only on Windows)
    if os.name == "nt":
        try:
            import winreg
            for key_root in (winreg.HKEY_LOCAL_MACHINE, winreg.HKEY_CURRENT_USER):
                for subkey in (r"SOFTWARE\Keil\MDK",
                               r"SOFTWARE\WOW6432Node\Keil\MDK",
                               r"SOFTWARE\ARM\RVCT"):
                    try:
                        with winreg.OpenKey(key_root, subkey) as reg_key:
                            value, _ = winreg.QueryValueEx(reg_key, "Path")
                            if value:
                                bin_dir = os.path.join(value, "ARM", "ARMCC", "bin")
                                if os.path.isfile(os.path.join(bin_dir, "armcc.exe")):
                                    return bin_dir
                    except (OSError, FileNotFoundError):
                        continue
        except ImportError:
            pass

    for p in _KNOWN_ARMCC_PATHS:
        if os.path.isfile(os.path.join(p, "armcc.exe")):
            return p
    return None
