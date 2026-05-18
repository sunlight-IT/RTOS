"""Shared utilities for the embedded CMake generator.

Consolidates logging, path normalization, file I/O, and monolithic
include detection that were previously duplicated across multiple modules.
"""

from __future__ import annotations

import os
import platform
import re
from typing import Dict, List, Optional

from .models import MonolithicInfo


# ---------------------------------------------------------------------------
# Logging (all output goes to stderr for clean stdout when using --json)
# ---------------------------------------------------------------------------
import sys

_LOG_TO_STDERR = False

def set_log_stderr(enabled: bool) -> None:
    """When True, log messages are written to stderr instead of stdout.

    This allows ``--json`` output to stdout without log pollution.
    """
    global _LOG_TO_STDERR  # noqa: PLW0603
    _LOG_TO_STDERR = enabled


def _log_out() -> sys.IOBase:
    """Return the appropriate output stream for log messages."""
    return sys.stderr if _LOG_TO_STDERR else sys.stdout


def log_info(msg: str) -> None:
    print(f"\033[0;32m[INFO]\033[0m {msg}", file=_log_out())


def log_warn(msg: str) -> None:
    print(f"\033[1;33m[WARN]\033[0m {msg}", file=_log_out())


def log_error(msg: str) -> None:
    print(f"\033[0;31m[ERROR]\033[0m {msg}", file=_log_out())


# ---------------------------------------------------------------------------
# Path utilities
# ---------------------------------------------------------------------------

def norm_path(path: str) -> str:
    """Normalize path separators to forward slashes."""
    return path.replace("\\", "/")


def source_rel_path(abs_path: str, project_dir: str) -> str:
    """Convert absolute source path to relative forward-slash path."""
    return norm_path(os.path.relpath(abs_path, project_dir))


# ---------------------------------------------------------------------------
# File I/O
# ---------------------------------------------------------------------------

def read_file_dual_encoding(filepath: str) -> Optional[str]:
    """Read a file trying GB2312 first, then UTF-8.

    Many Keil source files are saved in GB2312 (Chinese locale).
    """
    for encoding in ("gb2312", "utf-8"):
        try:
            with open(filepath, "r", encoding=encoding, errors="ignore") as f:
                return f.read()
        except (IOError, UnicodeDecodeError):
            continue
    return None


def write_file(path: str, content: str) -> None:
    """Write UTF-8 content to a file."""
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(content)


# ---------------------------------------------------------------------------
# Monolithic include detection
# ---------------------------------------------------------------------------

# Regex for ``#include "some_file.c"``
_MONOLITHIC_RE = re.compile(r'#include\s+"([^"]+\.c)"')


def detect_monolithic_includes(
    c_sources: List[str],
    project_dir: str = "",
) -> MonolithicInfo:
    """Detect ``.c`` files that ``#include`` other ``.c`` files.

    In some embedded projects (especially Keil-based), ``.c`` files directly
    ``#include`` other ``.c`` files, forming monolithic compilation units.
    The leaf (included) files should NOT be compiled separately.

    Args:
        c_sources: Absolute paths to C source files.
        project_dir: Project root path (used for leaf-file existence checks
                     when the source is a relative path).

    Returns:
        MonolithicInfo with root/leaf/mapping.
    """
    root_files: List[str] = []
    leaf_set: set = set()
    mapping: Dict[str, List[str]] = {}

    for src_file in c_sources:
        if not os.path.isfile(src_file):
            continue
        src_dir = os.path.dirname(src_file)

        content = read_file_dual_encoding(src_file)
        if content is None:
            continue

        found = _MONOLITHIC_RE.findall(content)
        if found:
            root_files.append(src_file)
            leaves: List[str] = []
            for inc_name in found:
                target = os.path.normpath(os.path.join(src_dir, inc_name))
                leaf_set.add(target)
                leaves.append(target)
            mapping[src_file] = leaves

    existing_leaves = sorted(lf for lf in leaf_set if os.path.isfile(lf))

    return MonolithicInfo(
        root_files=sorted(root_files),
        leaf_files=existing_leaves,
        mapping=mapping,
    )


# ---------------------------------------------------------------------------
# Sandbox detection
# ---------------------------------------------------------------------------

def is_sandboxed() -> bool:
    """Detect whether we are running inside a sandboxed/container environment.

    ARMCC builds require native Windows host execution. Returns True when
    running on non-Windows, inside Docker, or inside a Claude Code sandbox.
    """
    if platform.system() != "Windows":
        return True
    if os.environ.get("CLAUDE_CODE_SANDBOX"):
        return True
    if os.path.exists("/.dockerenv"):
        return True
    return False


def warn_monolithic_includes(info: MonolithicInfo, project_dir: str = "") -> None:
    """Print warnings for detected monolithic includes."""
    for leaf in info.leaf_files:
        rel_leaf = os.path.relpath(leaf, project_dir) if project_dir else leaf
        roots = info.mapping.keys()
        matching_roots = [
            os.path.relpath(r, project_dir) if project_dir else r
            for r in roots
            if leaf in info.mapping.get(r, [])
        ]
        rel_roots = ", ".join(matching_roots[:3])
        log_warn(
            f"Monolithic include detected: '{rel_leaf}' "
            f"is included by '{rel_roots}'"
            f"{'...' if len(matching_roots) > 3 else ''} "
            f"-- excluding from compilation."
        )
