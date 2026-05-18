"""Makefile parser for extracting project variables.

Parses Makefiles to extract TARGET, source file lists, and other variables
that help auto-detect project configuration.
"""

from __future__ import annotations

import os
import re
from typing import Dict, List, Optional


def parse_makefile(filepath: str) -> Dict[str, str]:
    """Parse a Makefile and extract variable assignments.

    Handles simple assignments (=, :=, +=) and continuation lines (\\).

    Args:
        filepath: Path to the Makefile.

    Returns:
        Dictionary of variable name to value.
    """
    result: Dict[str, str] = {}
    if not os.path.isfile(filepath):
        return result

    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    # Handle backslash-continuation lines
    content = re.sub(r"\\\n", " ", content)

    current_var = None
    current_value = ""
    current_op = "="

    for line in content.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue

        # Match variable assignment
        match = re.match(r"^(\w+)\s*([:+]?)=\s*(.*)", line)
        if match:
            if current_var:
                result[current_var] = current_value.strip()
            current_var = match.group(1)
            current_op = match.group(2) or "="
            current_value = match.group(3)
        elif current_var and line:
            # Continuation line (after backslash was joined)
            if current_op == "+=":
                current_value += " " + line
            else:
                current_value += " " + line

    if current_var:
        result[current_var] = current_value.strip()

    return result


def extract_target(mk_data: Dict[str, str]) -> Optional[str]:
    """Extract TARGET name from Makefile data."""
    return mk_data.get("TARGET")


def extract_defines(mk_data: Dict[str, str]) -> List[str]:
    """Extract ``-D`` preprocessor defines from Makefile variables.

    Checks ``CFLAGS``, ``DEFINES``, ``ASMFLAGS``, ``CXXFLAGS``, and
    ``AFLAGS`` for ``-DNAME`` / ``-DNAME=VALUE`` patterns.

    Args:
        mk_data: Parsed Makefile variables.

    Returns:
        List of define strings (``NAME`` or ``NAME=VALUE``).
    """
    defines: List[str] = []
    for key in ("CFLAGS", "DEFINES", "ASMFLAGS", "CXXFLAGS", "AFLAGS"):
        value = mk_data.get(key, "")
        for match in re.finditer(r"-D(\w+)(?:=(.+?))?(?=\s|$)", value):
            name = match.group(1)
            val = match.group(2)
            if val:
                defines.append(f"{name}={val}")
            else:
                defines.append(name)
    return defines


def extract_cpu_flags(mk_data: Dict[str, str]) -> Dict[str, str]:
    """Extract CPU architecture flags from Makefile compiler variables.

    Extracts ``-mcpu``, ``-mfpu``, and ``-mfloat-abi`` values.

    Args:
        mk_data: Parsed Makefile variables.

    Returns:
        Dict with optional keys ``cpu``, ``fpu``, ``float_abi``.
    """
    result: Dict[str, str] = {}
    for key in ("CFLAGS", "ASMFLAGS", "CXXFLAGS", "AFLAGS"):
        value = mk_data.get(key, "")
        m = re.search(r"-mcpu=(\S+)", value)
        if m:
            result["cpu"] = m.group(1)
        m = re.search(r"-mfpu=(\S+)", value)
        if m:
            result["fpu"] = m.group(1)
        m = re.search(r"-mfloat-abi=(\S+)", value)
        if m:
            result["float_abi"] = m.group(1)
    return result


def find_makefile(project_dir: str) -> Optional[str]:
    """Find a Makefile in the project directory.

    Checks for 'Makefile' (no extension) first, then 'GNUmakefile'.
    """
    for name in ["Makefile", "GNUmakefile"]:
        path = os.path.join(project_dir, name)
        if os.path.isfile(path):
            return path
    return None
