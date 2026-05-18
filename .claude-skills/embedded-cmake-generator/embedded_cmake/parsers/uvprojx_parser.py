"""Keil MDK .uvprojx / .uvproj project file parser.

Extracts compiler defines, include paths, source file groups,
scatter file, pre-compiled libraries, and other build settings
from Keil uVision project files.
"""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple
from xml.etree import ElementTree as ET

from ..models import KeilSourceGroup, MonolithicInfo
from ..utils import log_warn


def _create_safe_parser() -> ET.XMLParser:
    """Create an XML parser with external entity expansion disabled (XXE prevention).

    Uses ``resolve_entities=False`` on Python 3.8-3.13. On newer or older Python
    where the parameter is unsupported, falls back to the default parser — the
    Expat-based parser in CPython 3.7.1+ blocks external entities by default.
    """
    try:
        return ET.XMLParser(resolve_entities=False)
    except TypeError:
        # Fallback for Python versions where resolve_entities is not accepted
        return ET.XMLParser()


def find_uvprojx_file(project_dir: str) -> Optional[str]:
    """Search for a Keil .uvprojx or .uvproj file in common locations.

    Priority order:
    1. {project_dir}/*.uvprojx
    2. {project_dir}/*.uvproj
    3. {project_dir}/KeilPrj/*.uvprojx
    4. {project_dir}/MDK-ARM/*.uvproj*
    5. {project_dir}/**/KeilPrj/*.uvprojx  (recursive, depth <= 5)
    6. {project_dir}/build/**/*.uvprojx  (recursive)
    """
    project_path = Path(project_dir)

    # Priority 1-4: direct searches
    candidates = [
        list(project_path.glob("*.uvprojx")),
        list(project_path.glob("*.uvproj")),
        list(project_path.glob("KeilPrj/*.uvprojx")),
        list(project_path.glob("MDK-ARM/*.uvprojx")),
        list(project_path.glob("MDK-ARM/*.uvproj")),
    ]

    for cand_list in candidates:
        if cand_list:
            return str(cand_list[0])

    # Priority 5-6: recursive search (limited depth)
    for search_dir in ["KeilPrj", "build", "**"]:
        try:
            for depth in range(1, 6):
                pattern = "/".join(["*"] * depth + [search_dir, "*.uvprojx"])
                found = list(project_path.glob(pattern))
                if found:
                    return str(found[0])
        except (OSError, PermissionError):
            continue

    return None


def parse_uvprojx(filepath: str, project_dir: Optional[str] = None) -> Dict[str, Any]:
    """Parse a Keil .uvprojx file and extract all build configuration.

    Args:
        filepath: Path to .uvprojx file.
        project_dir: Project root directory. If None, inferred from filepath.

    Returns a dictionary with keys: device, vendor, cpu_type, defines,
    include_paths, groups, source_files, header_files, excluded_files,
    lib_files, scatter_file, toolchain_version, output_name, etc.
    """
    if not os.path.isfile(filepath):
        raise FileNotFoundError(f"Keil project file not found: {filepath}")

    tree = ET.parse(filepath, parser=_create_safe_parser())
    root = tree.getroot()
    uvprojx_dir = os.path.dirname(os.path.abspath(filepath))
    if project_dir is None:
        project_dir = _infer_project_dir(filepath)

    data: Dict[str, Any] = {
        "device": "",
        "vendor": "",
        "cpu_type": "Cortex-M3",
        "irom": {"origin": "0x08000000", "size": "0x80000"},
        "iram": {"origin": "0x20000000", "size": "0x10000"},
        "defines": [],
        "include_paths": [],
        "groups": [],
        "source_files": [],
        "header_files": [],
        "excluded_files": [],
        "lib_files": [],
        "scatter_file": "",
        "toolchain_version": "",
        "uses_microlib": True,
        "compiler_standard": "C99",
        "optimization": "-O1",
        "gnu_mode": False,
        "output_name": "",
        "monolithic": None,
        "asm_files": [],
    }

    # Find the primary target
    targets = root.findall(".//Targets/Target")
    if not targets:
        return data

    target = targets[0]

    # Extract device info
    _extract_device_info(target, data)

    # Extract compiler settings
    _extract_compiler_settings(target, uvprojx_dir, data)

    # Extract assembler settings
    _extract_assembler_settings(target, data)

    # Extract linker settings
    _extract_linker_settings(target, uvprojx_dir, project_dir, data)

    # Extract source file groups
    _extract_source_groups(target, uvprojx_dir, project_dir, data)

    # Extract after-make (post-build) steps
    _extract_after_make(target, data)

    # Detect monolithic includes in all C source files
    data["monolithic"] = _detect_monolithic_includes(data["source_files"])

    return data


def extract_defines(data: Dict[str, Any]) -> List[str]:
    """Get compiler defines from parsed Keil data."""
    return data.get("defines", [])


def extract_include_paths(data: Dict[str, Any]) -> List[str]:
    """Get include paths from parsed Keil data."""
    return data.get("include_paths", [])


def extract_source_files(data: Dict[str, Any]) -> List[str]:
    """Get all source files from parsed Keil data."""
    return data.get("source_files", [])


def extract_lib_files(data: Dict[str, Any]) -> List[str]:
    """Get pre-compiled library files from parsed Keil data."""
    return data.get("lib_files", [])


def extract_scatter_file(data: Dict[str, Any]) -> str:
    """Get scatter file path from parsed Keil data."""
    return data.get("scatter_file", "")


def extract_device_info(data: Dict[str, Any]) -> Tuple[str, str, str]:
    """Get device, vendor, and CPU type from parsed Keil data."""
    return (data.get("device", ""), data.get("vendor", ""), data.get("cpu_type", ""))


def extract_toolchain_info(data: Dict[str, Any]) -> Dict[str, Any]:
    """Get toolchain version and settings from parsed Keil data."""
    return {
        "version": data.get("toolchain_version", ""),
        "uses_microlib": data.get("uses_microlib", True),
        "compiler_standard": data.get("compiler_standard", "C99"),
        "optimization": data.get("optimization", "-O1"),
        "gnu_mode": data.get("gnu_mode", False),
    }


def detect_rtos_name(data: Dict[str, Any]) -> str:
    """Detect RTOS type from Keil project data."""
    defines = data.get("defines", [])
    for d in defines:
        if "FSL_RTOS_UCOSII" in d or "OS_CFG" in d or "UCOS" in d.upper():
            return "uCOS-II"
        if "FREERTOS" in d.upper():
            return "FreeRTOS"
    return ""


def _extract_device_info(target: ET.Element, data: Dict[str, Any]) -> None:
    """Extract device, vendor, and CPU info from target."""
    tco = target.find("./TargetOption/TargetCommonOption")
    if tco is None:
        return

    device_el = tco.find("Device")
    if device_el is not None and device_el.text:
        data["device"] = device_el.text.strip()

    vendor_el = tco.find("Vendor")
    if vendor_el is not None and vendor_el.text:
        data["vendor"] = vendor_el.text.strip()

    cpu_el = tco.find("Cpu")
    if cpu_el is not None and cpu_el.text:
        cpu_text = cpu_el.text.strip()
        # Parse CPU info: IRAM(addr,size) IROM(addr,size) CPUTYPE("Cortex-M3") CLOCK(12000000) ELITTLE
        iram_m = re.search(r"IRAM\(([^,]+),([^)]+)\)", cpu_text)
        irom_m = re.search(r"IROM\(([^,]+),([^)]+)\)", cpu_text)
        cpu_m = re.search(r'CPUTYPE\("([^"]+)"\)', cpu_text)
        clock_m = re.search(r"CLOCK\((\d+)\)", cpu_text)

        if iram_m:
            data["iram"] = {"origin": iram_m.group(1), "size": iram_m.group(2)}
        if irom_m:
            data["irom"] = {"origin": irom_m.group(1), "size": irom_m.group(2)}
        if cpu_m:
            data["cpu_type"] = cpu_m.group(1)
        if clock_m:
            data["cpu_clock_hz"] = int(clock_m.group(1))


def _extract_compiler_settings(target: ET.Element, uvprojx_dir: str, data: Dict[str, Any]) -> None:
    """Extract C/C++ compiler settings."""
    cads_parent = target.find("./TargetOption/TargetArmAds/Cads")
    if cads_parent is None:
        return

    various = cads_parent.find("VariousControls")

    # Defines
    if various is not None:
        define_el = various.find("Define")
        if define_el is not None and define_el.text:
            defines = [d.strip() for d in define_el.text.split(",") if d.strip()]
            data["defines"] = defines

        # Include paths
        inc_el = various.find("IncludePath")
        if inc_el is not None and inc_el.text:
            paths = []
            for p in inc_el.text.split(";"):
                p = p.strip()
                if not p:
                    continue
                # Resolve relative to .uvprojx directory
                resolved = _resolve_path(p, uvprojx_dir)
                if resolved and os.path.isdir(resolved):
                    paths.append(resolved)
            data["include_paths"] = paths

        # Misc controls
        misc_el = various.find("MiscControls")
        if misc_el is not None and misc_el.text and misc_el.text.strip():
            data["misc_compiler_flags"] = misc_el.text.strip()

    # Optimization level (child of <Cads>, not <VariousControls>)
    optim_el = cads_parent.find("Optim")
    if optim_el is not None and optim_el.text:
        try:
            opt_level = int(optim_el.text)
            data["optimization"] = f"-O{opt_level}" if opt_level > 0 else "-O0"
        except ValueError:
            pass

    # C standard (child of <Cads>, not <VariousControls>)
    c99_el = cads_parent.find("uC99")
    if c99_el is not None and c99_el.text == "1":
        data["compiler_standard"] = "C99"

    # GNU extensions mode (child of <Cads>, not <VariousControls>)
    gnu_el = cads_parent.find("uGnu")
    if gnu_el is not None and gnu_el.text == "1":
        data["gnu_mode"] = True


def _extract_assembler_settings(target: ET.Element, data: Dict[str, Any]) -> None:
    """Extract assembler settings."""
    aads = target.find("./TargetOption/TargetArmAds/Aads/VariousControls")
    if aads is None:
        return

    define_el = aads.find("Define")
    if define_el is not None and define_el.text:
        asm_defines = [d.strip() for d in define_el.text.split(",") if d.strip()]
        if asm_defines:
            data["asm_defines"] = asm_defines


def _extract_linker_settings(
    target: ET.Element, uvprojx_dir: str, project_dir: str, data: Dict[str, Any]
) -> None:
    """Extract linker settings."""
    ldads = target.find("./TargetOption/TargetArmAds/LDads")
    if ldads is None:
        return

    # Scatter file
    scatter_el = ldads.find("ScatterFile")
    if scatter_el is not None and scatter_el.text:
        resolved = _resolve_path(scatter_el.text.strip(), uvprojx_dir)
        data["scatter_file"] = _to_project_relative(resolved, project_dir)

    # MicroLIB
    ulib_el = ldads.find("useUlib")
    if ulib_el is not None:
        data["uses_microlib"] = ulib_el.text == "1"

    # Misc linker controls
    misc_el = ldads.find("Misc")
    if misc_el is not None and misc_el.text:
        data["misc_linker_flags"] = misc_el.text.strip()


def _extract_source_groups(
    target: ET.Element, uvprojx_dir: str, project_dir: str, data: Dict[str, Any]
) -> None:
    """Extract source file groups with per-file include/exclude flags."""
    groups = target.findall("./Groups/Group")

    for group in groups:
        group_name_el = group.find("GroupName")
        group_name = group_name_el.text if group_name_el is not None else ""

        source_group = KeilSourceGroup(group_name=group_name)
        file_elements = group.findall("Files/File")

        for fe in file_elements:
            file_path_el = fe.find("FilePath")
            file_type_el = fe.find("FileType")
            include_build_el = fe.find(".//IncludeInBuild")  # recursive: FileOption/CommonProperty/IncludeInBuild

            if file_path_el is None:
                continue

            file_path = file_path_el.text.strip()
            file_type = int(file_type_el.text) if file_type_el is not None and file_type_el.text else 0
            include_in_build = (
                int(include_build_el.text) if include_build_el is not None and include_build_el.text else 2
            )

            resolved = _resolve_path(file_path, uvprojx_dir)
            if not resolved or not os.path.isfile(resolved):
                # Try project root as fallback (Keil projects often reference
                # files relative to the project root rather than uvprojx dir)
                resolved2 = _resolve_path(file_path, project_dir)
                if resolved2 and os.path.isfile(resolved2):
                    resolved = resolved2
                else:
                    continue

            # Path scope check: allow files within project_dir or up to 3 ancestor levels
            # (handles monorepo patterns like ../../Libraries/CMSIS/...).
            # Cross-drive paths are still rejected (Windows security).
            if not _is_path_within_scope(resolved, project_dir, max_up=3):
                log_warn(f"Skipping path outside project scope: {resolved}")
                continue

            rel_path = _to_project_relative(resolved, project_dir)

            # FileType: 1=C Source, 2=Asm Source, 4=Library, 5=Header/Other
            if include_in_build == 0:
                source_group.excluded_files.append(rel_path)
                data["excluded_files"].append(rel_path)
            elif file_type == 4:  # Library
                source_group.lib_files.append(rel_path)
                data["lib_files"].append(rel_path)
            elif file_type == 5:  # Header
                source_group.header_files.append(rel_path)
                data["header_files"].append(rel_path)
            elif file_type in (1, 2):
                source_group.source_files.append(rel_path)
                if rel_path not in data["source_files"]:
                    data["source_files"].append(rel_path)
                if file_type == 2:
                    if rel_path not in data["asm_files"]:
                        data["asm_files"].append(rel_path)

        data["groups"].append(source_group)


def _extract_after_make(target: ET.Element, data: Dict[str, Any]) -> None:
    """Extract post-build steps."""
    tco = target.find("./TargetOption/TargetCommonOption")
    if tco is None:
        return

    output_name_el = tco.find("OutputName")
    if output_name_el is not None and output_name_el.text:
        data["output_name"] = output_name_el.text.strip()

    after_make = tco.find("AfterMake")
    if after_make is not None:
        prog1 = after_make.find("UserProg1Name")
        if prog1 is not None and prog1.text:
            data["post_build_step_1"] = prog1.text.strip()
        prog2 = after_make.find("UserProg2Name")
        if prog2 is not None and prog2.text:
            data["post_build_step_2"] = prog2.text.strip()


def _detect_monolithic_includes(source_files: List[str]) -> MonolithicInfo:
    """Detect .c files that #include other .c files (monolithic build pattern)."""
    include_pattern = re.compile(r'#include\s+"([^"]+\.c)"')

    root_files: List[str] = []
    leaf_set: set = set()
    mapping: Dict[str, List[str]] = {}

    for src_file in source_files:
        if not os.path.isfile(src_file):
            continue
        src_dir = os.path.dirname(src_file)

        try:
            with open(src_file, "r", encoding="gb2312", errors="ignore") as f:
                content = f.read()
        except (IOError, UnicodeDecodeError):
            try:
                with open(src_file, "r", encoding="utf-8", errors="ignore") as f:
                    content = f.read()
            except (IOError, UnicodeDecodeError):
                continue

        found = include_pattern.findall(content)
        if found:
            root_files.append(src_file)
            leaves = []
            for inc_name in found:
                target = os.path.normpath(os.path.join(src_dir, inc_name))
                leaf_set.add(target)
                leaves.append(target)
            mapping[src_file] = leaves

    # Only include leaves that exist on disk
    existing_leaves = sorted(lf for lf in leaf_set if os.path.isfile(lf))

    return MonolithicInfo(
        root_files=sorted(root_files),
        leaf_files=existing_leaves,
        mapping=mapping,
    )


def _resolve_path(filepath: str, uvprojx_dir: str) -> str:
    """Resolve a path that may be relative to the .uvprojx file."""
    if os.path.isabs(filepath):
        return os.path.normpath(filepath)
    resolved = os.path.normpath(os.path.join(uvprojx_dir, filepath))
    return resolved


def _is_path_within_scope(resolved_path: str, project_dir: str, max_up: int = 3) -> bool:
    """Check if a resolved path is within project scope.

    Allows paths within ``project_dir`` or up to ``max_up`` ancestor
    levels (handles monorepo patterns e.g. ``../../Libraries/``).
    Cross-drive paths are rejected (Windows security).

    Args:
        resolved_path: Absolute path to check.
        project_dir: Expected project root directory.
        max_up: How many parent-directory levels above project_dir to allow.

    Returns:
        True if the path is within the allowed scope.
    """
    try:
        abs_path = os.path.abspath(resolved_path)
        abs_project = os.path.abspath(project_dir)
        common = os.path.commonpath([abs_path, abs_project])
        # Walk up from project_dir checking for a common ancestor
        for _ in range(max_up + 1):
            if common == abs_project:
                return True
            abs_project = os.path.dirname(abs_project)
        return False
    except ValueError:
        # Different drives on Windows — always outside scope
        return False


def _to_project_relative(resolved_path: str, project_dir: str) -> str:
    """Convert an absolute path to project-relative path."""
    try:
        return os.path.normpath(os.path.relpath(resolved_path, project_dir))
    except ValueError:
        return resolved_path


def _infer_project_dir(filepath: str) -> str:
    """Infer the project root directory from the .uvprojx path."""
    uvprojx_dir = os.path.dirname(os.path.abspath(filepath))

    # Walk up to find the project root (where source/ or CMakeLists.txt lives)
    current = os.path.abspath(uvprojx_dir)
    for _ in range(6):
        if os.path.isfile(os.path.join(current, "CMakeLists.txt")):
            return current
        if os.path.isdir(os.path.join(current, "source")):
            return current
        parent = os.path.dirname(current)
        if parent == current:
            break
        current = parent

    return uvprojx_dir
