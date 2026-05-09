"""Configuration file loader, validator, and merger.

Loads embedded-cmake.json from project root, validates structure,
and merges with auto-detected values. Auto-detected values are
overridden by user-provided config values.
"""

from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

from .models import ProjectConfig, ScanConfig, RTOSConfig, ChipInfo
from .chip_db import ChipDB
from .toolchain import ToolchainRegistry
from .detector import ProjectDetector


CONFIG_FILENAME = "embedded-cmake.json"


def load_config(
    project_dir: str,
    chip_db: Optional[ChipDB] = None,
    toolchain_registry: Optional[ToolchainRegistry] = None,
) -> ProjectConfig:
    """Load project configuration.

    Steps:
    1. Auto-detect project structure
    2. If embedded-cmake.json exists, load and merge (user values override)
    3. Return merged ProjectConfig

    Args:
        project_dir: Root project directory.
        chip_db: Optional pre-configured chip database.
        toolchain_registry: Optional pre-configured toolchain registry.

    Returns:
        Merged ProjectConfig ready for generation.
    """
    detector = ProjectDetector(project_dir, chip_db, toolchain_registry)
    detected = detector.detect()

    # Try to load user config
    config_path = os.path.join(project_dir, CONFIG_FILENAME)
    if os.path.isfile(config_path):
        user_config = _load_json_config(config_path)
        merge_configs(detected, user_config, detector._chip_db)

    return detected


def _load_json_config(filepath: str) -> Dict[str, Any]:
    """Load and preprocess a JSON config file.

    Strips // and # comment lines for convenience.
    """
    with open(filepath, "r", encoding="utf-8") as f:
        lines = f.readlines()

    # Strip comment lines
    clean_lines = []
    for line in lines:
        stripped = line.strip()
        if stripped.startswith("//") or stripped.startswith("#"):
            continue
        clean_lines.append(line)

    return json.loads("\n".join(clean_lines))


def merge_configs(
    config: ProjectConfig,
    user_data: Dict[str, Any],
    chip_db: Optional[ChipDB] = None,
) -> None:
    """Merge user-provided config values into a ProjectConfig (in-place).

    User values override auto-detected values. List-type fields
    (defines, exclude_dirs, etc.) are extended, not replaced.

    Args:
        config: ProjectConfig to merge into (modified in place).
        user_data: Parsed user config JSON.
        chip_db: Optional chip database for resolving chip references.
    """
    # Project
    proj = user_data.get("project", {})
    if proj.get("name"):
        config.project_name = proj["name"]

    # Chip
    chip = user_data.get("chip", {})
    if chip.get("family"):
        config.chip_family = chip["family"]
    if chip.get("model"):
        config.chip_model = chip["model"]
    if (chip.get("family") or chip.get("model")) and chip_db:
        _resolve_chip(config, chip_db)

    # Scan
    scan_data = user_data.get("scan", {})
    if scan_data:
        _merge_scan_config(config.scan, scan_data)

    # Defines - extend
    user_defines = user_data.get("defines", [])
    if isinstance(user_defines, list):
        for d in user_defines:
            if d not in config.defines:
                config.defines.append(d)

    # Toolchains
    user_tcs = user_data.get("toolchains", [])
    if isinstance(user_tcs, list) and user_tcs:
        config.toolchains = user_tcs

    # ARMCC specific
    armcc = user_data.get("armcc", {})
    if armcc.get("compiler_dir"):
        config.armcc_compiler_dir = armcc["compiler_dir"]

    # RTOS
    rtos_data = user_data.get("rtos", {})
    if rtos_data:
        _merge_rtos_config(config.rtos, rtos_data)

    # Startup files
    if user_data.get("startup_file"):
        config.startup_file = user_data["startup_file"]
    if user_data.get("armcc_startup_file"):
        config.armcc_startup_file = user_data["armcc_startup_file"]

    # Linker
    if user_data.get("linker_script"):
        config.linker_script = user_data["linker_script"]
    if user_data.get("armcc_scatter_file"):
        config.armcc_scatter_file = user_data["armcc_scatter_file"]

    # Output
    output = user_data.get("output", {})
    if "cmake_dir" in output:
        config.cmake_dir = output["cmake_dir"]
    if "hex" in output:
        config.output_hex = output["hex"]
    if "bin" in output:
        config.output_bin = output["bin"]
    if "size" in output:
        config.output_size = output["size"]


def _resolve_chip(config: ProjectConfig, chip_db: ChipDB) -> None:
    """Resolve chip family/model to a ChipInfo from the database."""
    if config.chip_model:
        chip_info = chip_db.find_family_for_model(config.chip_model)
        if chip_info:
            config.chip_info = chip_info
            config.chip_family = chip_info.family
            # Add default defines
            for d in chip_info.default_defines:
                if d not in config.defines:
                    config.defines.append(d)
            # Add model-specific defines
            model = chip_info.get_model(config.chip_model)
            if model:
                for d in model.defines:
                    if d not in config.defines:
                        config.defines.append(d)
            return

    if config.chip_family:
        chip_info = chip_db.find_family(config.chip_family)
        if chip_info:
            config.chip_info = chip_info
            for d in chip_info.default_defines:
                if d not in config.defines:
                    config.defines.append(d)


def _merge_scan_config(scan: ScanConfig, data: Dict[str, Any]) -> None:
    """Merge user scan config into ScanConfig (in-place)."""
    if "source_extensions" in data:
        scan.source_extensions = data["source_extensions"]
    if "header_extensions" in data:
        scan.header_extensions = data["header_extensions"]
    # Lists are extended
    for key in ["exclude_dirs", "exclude_dir_patterns", "exclude_files",
                 "exclude_file_patterns", "extra_exclude_header_dirs"]:
        if key in data:
            for item in data[key]:
                if item not in getattr(scan, key):
                    getattr(scan, key).append(item)


def _merge_rtos_config(rtos: RTOSConfig, data: Dict[str, Any]) -> None:
    """Merge user RTOS config into RTOSConfig (in-place)."""
    if "type" in data:
        rtos.type = data["type"]
    if "heap" in data:
        rtos.heap = data["heap"]
    if "cmsis_version" in data:
        rtos.cmsis_version = data["cmsis_version"]
    if "base_path" in data:
        rtos.base_path = data["base_path"]
    if "port_mapping" in data:
        rtos.port_mapping.update(data["port_mapping"])
