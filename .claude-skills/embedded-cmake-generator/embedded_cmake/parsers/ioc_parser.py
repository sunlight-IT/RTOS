"""CubeMX .ioc file parser.

Parses STM32CubeMX project files (.ioc) to extract chip model, family,
RTOS type, compiler preferences, defines, and project metadata.
"""

from __future__ import annotations

import os
import re
from typing import Dict, List, Optional


def parse_ioc(filepath: str) -> Dict[str, str]:
    """Parse a CubeMX .ioc file and return key-value pairs.

    The .ioc format is a simple key=value file (INI-like).
    Multi-line values use backslash continuation.

    Args:
        filepath: Path to the .ioc file.

    Returns:
        Dictionary of all key-value pairs.
    """
    result: Dict[str, str] = {}
    if not os.path.isfile(filepath):
        return result

    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    # Handle backslash-continuation lines
    content = re.sub(r"\\\n\s*", "", content)

    for line in content.splitlines():
        line = line.strip()
        if not line or line.startswith("#") or line.startswith(";"):
            continue
        if "=" in line:
            key, _, value = line.partition("=")
            result[key.strip()] = value.strip()

    return result


def extract_chip_model(ioc_data: Dict[str, str]) -> Optional[str]:
    """Extract chip model name from .ioc data.

    Priority: Mcu.CPN > Mcu.UserName > Mcu.Name
    """
    name = ioc_data.get("Mcu.CPN")
    if name:
        return name
    name = ioc_data.get("Mcu.UserName")
    if name:
        return name
    name = ioc_data.get("Mcu.Name")
    if name:
        # CubeMX format: STM32F103C(8-B)Tx -> STM32F103C8Tx
        name = re.sub(r"\([^)]*\)", "", name)
        return name
    return None


def extract_chip_family(ioc_data: Dict[str, str]) -> Optional[str]:
    """Extract chip family from .ioc data.

    Uses Mcu.Family key.
    """
    return ioc_data.get("Mcu.Family")


def extract_project_name(ioc_data: Dict[str, str]) -> Optional[str]:
    """Extract project name from .ioc data."""
    return ioc_data.get("ProjectManager.ProjectName")


def extract_toolchain(ioc_data: Dict[str, str]) -> Optional[str]:
    """Extract preferred toolchain from .ioc data.

    Returns 'armcc', 'gcc', 'iar', or None.
    """
    tc = ioc_data.get("ProjectManager.TargetToolchain", "").upper()
    if "MDK-ARM" in tc or "KEIL" in tc:
        return "armcc"
    compiler = ioc_data.get("ProjectManager.CompilerLinker", "").upper()
    if "GCC" in compiler or "GNU" in compiler:
        return "gcc"
    if "IAR" in compiler or "ICCARM" in compiler:
        return "iar"
    return None


def extract_defines(ioc_data: Dict[str, str]) -> List[str]:
    """Extract compiler defines from .ioc data."""
    defines_str = ioc_data.get("CDefines", "")
    if defines_str:
        return [d.strip() for d in defines_str.split(";") if d.strip()]
    return []


def has_rtos(ioc_data: Dict[str, str]) -> Optional[str]:
    """Check if .ioc project has an RTOS configured.

    Returns 'FreeRTOS', 'ThreadX', or None.
    """
    # CubeMX stores RTOS as an IP with key like FREERTOS.IPParameters
    for key in ioc_data:
        if key.startswith("FREERTOS."):
            return "FreeRTOS"
        if key.startswith("THREADX."):
            return "ThreadX"
    # Also check Mcu.IP list
    ip_list = ioc_data.get("Mcu.IP4", "")
    if "FREERTOS" in ip_list:
        return "FreeRTOS"
    if "THREADX" in ip_list:
        return "ThreadX"
    return None


def find_ioc_file(project_dir: str) -> Optional[str]:
    """Find a CubeMX .ioc file in the project directory.

    Returns the path to the first .ioc file found, or None.
    """
    for f in os.listdir(project_dir):
        if f.endswith(".ioc"):
            return os.path.join(project_dir, f)
    return None


def find_cubemx_repo_root(ioc_filepath: str, max_depth: int = 8) -> Optional[str]:
    """Walk up from a .ioc file directory looking for a ``Drivers/`` root.

    CubeMX repositories follow a standard layout::

        <repo_root>/
            Drivers/
                STM32xx_HAL_Driver/
                CMSIS/
            Projects/
                <board>/<example>/

    Returns the repo root path, or ``None`` if not found within ``max_depth``
    parent directories.
    """
    current = os.path.dirname(os.path.abspath(ioc_filepath))
    for _ in range(max_depth):
        if os.path.isdir(os.path.join(current, "Drivers")):
            return current
        parent = os.path.dirname(current)
        if parent == current:
            break
        current = parent
    return None
