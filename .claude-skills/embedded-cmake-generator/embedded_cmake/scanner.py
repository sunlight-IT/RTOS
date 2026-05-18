"""Configurable source and header file scanner.

Scans project directories for C/C++/ASM source files and header directories,
with configurable include/exclude rules from ``ScanConfig``.
"""

from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any, Dict, List, Optional, Set

from .models import ScanConfig, ScanResult
from .utils import log_warn, read_file_dual_encoding


# ---------------------------------------------------------------------------
# JSON data loader
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


def _load_scan_defaults() -> Dict[str, Any]:
    """Load scan defaults from data/scan_defaults.json."""
    return _load_json_data("scan_defaults.json")


# ---------------------------------------------------------------------------
# Pattern matching
# ---------------------------------------------------------------------------

def _match_simple_pattern(name: str, pattern: str) -> bool:
    """Simple glob-style pattern matching (supports ``*`` and ``?``)."""
    if pattern == "*":
        return True
    if pattern.startswith("*") and pattern.endswith("*"):
        return pattern[1:-1] in name
    if pattern.startswith("*"):
        return name.endswith(pattern[1:])
    if pattern.endswith("*"):
        return name.startswith(pattern[:-1])
    if "?" in pattern:
        import fnmatch
        return fnmatch.fnmatch(name, pattern)
    return name == pattern


# ---------------------------------------------------------------------------
# Exclusion helpers
# ---------------------------------------------------------------------------

def _should_exclude_path(root: str, config: ScanConfig) -> bool:
    """Check if a directory path should be excluded from scanning."""
    parts = Path(root).parts
    for part in parts:
        if part in config.exclude_dirs:
            return True
        for pattern in config.exclude_dir_patterns:
            if _match_simple_pattern(part, pattern):
                return True
    return False


def _should_exclude_file(filename: str, config: ScanConfig) -> bool:
    """Check if a file should be excluded from scanning."""
    if filename in config.exclude_files:
        return True
    for pattern in config.exclude_file_patterns:
        if _match_simple_pattern(filename, pattern):
            return True
    return False


def _is_cmake_build_dir(dirpath: str) -> bool:
    """Check if a directory is a CMake build directory (contains CMakeCache.txt)."""
    return os.path.isfile(os.path.join(dirpath, "CMakeCache.txt"))


def _should_exclude_dir_name(dirname: str, config: ScanConfig) -> bool:
    """Check if a directory name (single component) should be excluded."""
    if dirname in config.exclude_dirs:
        return True
    for pattern in config.exclude_dir_patterns:
        if _match_simple_pattern(dirname, pattern):
            return True
    return False


# ---------------------------------------------------------------------------
# Chip-family-aware directory filtering
# ---------------------------------------------------------------------------


def _get_chip_family_prefix(chip_family: str) -> str:
    """Extract the base prefix from a chip family name for directory matching.

    ``STM32L1`` -> ``stm32l``
    ``APM32E1`` -> ``apm32e``
    """
    return chip_family.lower().rstrip("0123456789")


def _is_chip_family_dir(name: str) -> bool:
    """Heuristic: a directory is likely a chip family if it starts with
    uppercase and contains digits (e.g. ``STM32L``, ``APM32``, ``MK64F``).
    """
    return bool(name) and name[0].isupper() and any(c.isdigit() for c in name)


def _build_chip_family_filter(project_dir: str, chip_family: str) -> Optional[callable]:
    """Build a filter function that prunes foreign MCU directories during os.walk.

    Only filters immediate children of configured search paths (e.g. ``Hardware/``,
    ``Bsp/``). Subdirectories of kept paths pass through unfiltered.

    Returns a callable ``(root, dirs) -> None`` (modifies dirs in place), or None
    when filtering is disabled or the project has a single MCU.
    """
    if not chip_family:
        return None

    defaults = _load_scan_defaults()
    cfg = defaults.get("chip_family_filtering", {})
    if not cfg.get("enabled", True):
        return None

    raw_paths = cfg.get("search_paths", ["Hardware", "Bsp"])
    search_paths = []
    for sp in raw_paths:
        for candidate in (os.path.join(project_dir, sp),
                          os.path.join(project_dir, "source", sp)):
            if os.path.isdir(candidate):
                search_paths.append(os.path.normpath(candidate).lower())
    search_paths.sort()
    active_prefix = _get_chip_family_prefix(chip_family)

    def _filter(root: str, dirs: List[str]) -> None:
        root_lower = root.lower()
        # Only filter when root IS a search path (not subdirectories of it)
        for sp in search_paths:
            if root_lower == sp:
                dirs[:] = [
                    d for d in dirs
                    if not _is_chip_family_dir(d)
                    or d.lower().startswith(active_prefix)
                ]
                return

    return _filter


# ---------------------------------------------------------------------------
# Source scanning
# ---------------------------------------------------------------------------

def scan_sources(project_dir: str, config: Optional[ScanConfig] = None,
                 chip_family: str = "") -> ScanResult:
    """Scan project directory for source files.

    Args:
        project_dir: Root project directory path.
        config: Scan configuration. Uses defaults if not provided.
        chip_family: Active chip family name for filtering out foreign MCU
                     directories (e.g. ``STM32L1``). Empty = no filtering.

    Returns:
        ScanResult with categorized file lists.
    """
    if config is None:
        config = ScanConfig()

    chip_filter = _build_chip_family_filter(project_dir, chip_family)

    c_sources: List[str] = []
    cpp_sources: List[str] = []
    asm_sources: List[str] = []
    lib_files: List[str] = []
    static_lib_files: List[str] = []

    source_exts = set(config.source_extensions)
    ext_cats = _load_scan_defaults().get("extension_categories", {})
    cpp_exts = set(ext_cats.get("cpp", [".cpp", ".cxx", ".cc"]))
    asm_exts = set(ext_cats.get("asm", [".s", ".S", ".asm"]))
    lib_exts = set(ext_cats.get("lib", [".lib"]))
    static_lib_exts = set(ext_cats.get("static_lib", [".a"]))

    for root, dirs, files in os.walk(project_dir):
        # Auto-detect and exclude CMake build directories
        dirs[:] = [d for d in dirs if not _is_cmake_build_dir(os.path.join(root, d))]
        # Apply directory exclusions
        dirs[:] = [d for d in dirs if not _should_exclude_dir_name(d, config)]
        # Apply chip-family filtering (exclude foreign MCU directories)
        if chip_filter:
            chip_filter(root, dirs)

        if _should_exclude_path(root, config):
            continue

        for file in files:
            if _should_exclude_file(file, config):
                continue

            ext_match = None
            for ext in source_exts:
                if file.endswith(ext):
                    ext_match = ext
                    break

            if ext_match is None:
                # Check for library files (.lib, .a)
                filepath = os.path.normpath(os.path.join(root, file))
                if any(file.endswith(ext) for ext in lib_exts):
                    lib_files.append(filepath)
                elif any(file.endswith(ext) for ext in static_lib_exts):
                    static_lib_files.append(filepath)
                continue

            filepath = os.path.normpath(os.path.join(root, file))

            if ext_match in asm_exts:
                asm_sources.append(filepath)
            elif ext_match in cpp_exts:
                cpp_sources.append(filepath)
            else:
                c_sources.append(filepath)

    return ScanResult(
        c_sources=sorted(c_sources),
        cpp_sources=sorted(cpp_sources),
        asm_sources=sorted(asm_sources),
        lib_files=sorted(lib_files),
        static_lib_files=sorted(static_lib_files),
    )


# ---------------------------------------------------------------------------
# Header scanning
# ---------------------------------------------------------------------------

def scan_headers(
    project_dir: str,
    config: Optional[ScanConfig] = None,
    extra_exclude_subdirs: Optional[List[str]] = None,
    chip_family: str = "",
) -> List[str]:
    """Scan project directory for directories containing header files.

    Args:
        project_dir: Root project directory path.
        config: Scan configuration. Uses defaults if not provided.
        extra_exclude_subdirs: Additional subdir names to exclude from header paths.
        chip_family: Active chip family name for filtering out foreign MCU
                     directories (e.g. ``STM32L1``). Empty = no filtering.

    Returns:
        Sorted list of directories containing header files.
    """
    if config is None:
        config = ScanConfig()

    chip_filter = _build_chip_family_filter(project_dir, chip_family)

    exclude_subdirs = list(config.extra_exclude_header_dirs)
    if extra_exclude_subdirs:
        exclude_subdirs.extend(extra_exclude_subdirs)

    header_dirs: Set[str] = set()
    header_exts = set(config.header_extensions)

    for root, dirs, files in os.walk(project_dir):
        dirs[:] = [d for d in dirs if not _is_cmake_build_dir(os.path.join(root, d))]
        dirs[:] = [d for d in dirs if not _should_exclude_dir_name(d, config)]
        dirs[:] = [d for d in dirs if d not in exclude_subdirs]
        # Apply chip-family filtering (exclude foreign MCU directories)
        if chip_filter:
            chip_filter(root, dirs)

        if _should_exclude_path(root, config):
            continue

        for file in files:
            for ext in header_exts:
                if file.endswith(ext):
                    header_dirs.add(os.path.normpath(root))
                    break

    return sorted(header_dirs)


# ---------------------------------------------------------------------------
# config.h conflict detection
# ---------------------------------------------------------------------------

def detect_config_h_conflicts(
    header_dirs: List[str],
    project_dir: str,
) -> List[str]:
    """Detect and prioritize directories containing config.h files.

    Ensures that source/Main/config.h (project-level) takes priority over
    source/Common/ToolsEx/config.h (utility-level) and any other config.h.

    Returns:
        Reordered header_dirs with high-priority config.h directories first.
    """
    dirs_with_config_h = [
        d for d in header_dirs
        if os.path.isfile(os.path.join(d, "config.h"))
    ]

    if len(dirs_with_config_h) <= 1:
        return header_dirs

    def priority_key(d: str) -> int:
        parts = Path(d).parts
        indicators = _load_scan_defaults().get("config_h_priority_indicators",
                                                ["Main", "main", "App", "Inc"])
        for i, indicator in enumerate(indicators):
            if indicator in parts:
                return i
        return len(parts) + 100

    sorted_config_dirs = sorted(dirs_with_config_h, key=priority_key)
    primary = sorted_config_dirs[0]

    for shadowed in sorted_config_dirs[1:]:
        rel_p = os.path.relpath(primary, project_dir)
        rel_s = os.path.relpath(shadowed, project_dir)
        log_warn(f"Multiple config.h files detected: '{rel_s}' is shadowed by '{rel_p}'.")

    config_set = set(dirs_with_config_h)
    non_config_dirs = [d for d in header_dirs if d not in config_set]
    other_config_dirs = [d for d in sorted_config_dirs[1:] if d in header_dirs]

    return [primary] + non_config_dirs + other_config_dirs


# ---------------------------------------------------------------------------
# Feature flag analysis (diagnostic)
# ---------------------------------------------------------------------------

import re as _re


def analyze_feature_flags(project_dir: str, config_h_dir: str,
                           defines: List[str]) -> List[Dict[str, Any]]:
    """Scan config.h for feature flags that are 0 by default or commented out.

    Args:
        project_dir: Root project directory (unused, kept for API consistency).
        config_h_dir: Directory containing the primary config.h (from
                      ``detect_config_h_conflicts``).
        defines: Project defines list (from ``ProjectConfig.defines``).

    Returns:
        List of ``{flag, value, detail}`` dicts for potentially missing flags.
    """
    config_h_path = os.path.join(config_h_dir, "config.h")
    if not os.path.isfile(config_h_path):
        return []

    try:
        with open(config_h_path, "r", encoding="utf-8", errors="replace") as f:
            config_h_content = f.read()
    except OSError:
        return []

    defined_set = set(defines)
    # Normalise: strip ``-D`` / ``-D <flag>=<val>`` syntax
    for d in defines:
        if d.startswith("-D"):
            defined_set.add(d[2:].split("=")[0])

    results: List[Dict[str, Any]] = []

    # Pattern 1: ``#define __XXX 0``  (explicitly disabled)
    for m in _re.finditer(
            r'#define\s+(__[A-Z][A-Z0-9_]+)\s+0\s*(?://.*)?$',
            config_h_content,
            _re.MULTILINE,
    ):
        flag = m.group(1)
        if flag not in defined_set:
            results.append({
                "flag": flag,
                "value": 0,
                "detail": f"#define {flag} 0 (disabled by default)",
            })

    # Pattern 2: ``//#define __XXX 1``  (commented-out enable)
    for m in _re.finditer(
            r'(?://|/\*)\s*#define\s+(__[A-Z][A-Z0-9_]+)\s+1',
            config_h_content,
    ):
        flag = m.group(1)
        if flag not in defined_set:
            results.append({
                "flag": flag,
                "value": "commented_out",
                "detail": f"Commented out: #define {flag} 1",
            })

    # Pattern 3: ``#ifndef __XXX`` / ``#define __XXX 0`` guard
    for m in _re.finditer(
            r'#ifndef\s+(__[A-Z][A-Z0-9_]+)\s*\n\s*#define\s+\1\s+0',
            config_h_content,
    ):
        flag = m.group(1)
        if flag not in defined_set:
            results.append({
                "flag": flag,
                "value": 0,
                "detail": f"#ifndef {flag} → #define {flag} 0",
            })

    return results


# ---------------------------------------------------------------------------
# Board feature flag suggestion (diagnostic)
# ---------------------------------------------------------------------------


def _evaluate_board_condition(condition: str, active_board_value: int,
                               board_map: Dict[str, int]) -> bool:
    """Evaluate a preprocessor ``#if`` condition for board matching.

    Handles patterns like::

        #if (__BOARD == BOARD_WLM200) || (__BOARD == BOARD_WS100)

    Returns ``True`` when the active board matches any ``BOARD_XXX`` in the
    condition.  Returns ``False`` for non-``__BOARD`` conditions (they are
    evaluated conservatively — only board-defined branches are taken).
    """
    cond = condition.strip()

    # Simple constants
    if cond == "1":
        return True
    if cond == "0":
        return False

    # Only evaluate __BOARD == BOARD_XXX patterns
    if "__BOARD" not in cond:
        return False

    # Split on ``||`` and search each part for ``__BOARD == BOARD_XXX``.
    # Use ``search`` (not ``match``) to tolerate left-over parens — unbalanced
    # parens can occur when the condition has nested ``((...))`` wrapping.
    parts = _re.split(r'\s*\|\|\s*', cond)
    for part in parts:
        m = _re.search(r'__BOARD\s*==\s*(BOARD_\w+)', part)
        if m:
            board_name = m.group(1)
            if board_map.get(board_name) == active_board_value:
                return True

    return False


def suggest_board_features(project_dir: str, config_h_dir: str,
                           defines: List[str]) -> Dict[str, Any]:
    """Analyze ``config.h`` for board-specific feature flag suggestions.

    Parses ``config.h`` to find:

    1. Board definitions (``BOARD_XXX = N``).
    2. Active ``__BOARD`` value (from *defines* or ``config.h`` default).
    3. Board-specific ``#if (__BOARD == BOARD_XXX)`` blocks matching the
       active board.
    4. Feature flags that are enabled (=1), guarded (``#ifndef``), or
       commented out inside those blocks.
    5. Flags referenced in preprocessor conditions but never defined
       anywhere in ``config.h``.

    The results help a user know which ``-D`` flags to add to their
    ``embedded-cmake.json`` when migrating from a Keil IDE project that
    previously supplied those defines externally.

    Args:
        project_dir: Root project directory (unused, API compatibility).
        config_h_dir: Directory containing the primary ``config.h``.
        defines: Project defines list (from ``ProjectConfig.defines``).

    Returns:
        Dict with keys:

        - **board_name** — resolved board name (e.g. ``"BOARD_WLM200"``).
        - **board_value** — numeric board value.
        - **must_define** — ``[(flag, reason), ...]`` for flags that should
          be defined (=1) but are currently missing.
        - **never_defined** — ``[(flag, reason), ...]`` for ``__XXX`` flags
          referenced in preprocessor conditions but never defined in
          ``config.h``.
        - **board_map** — all ``BOARD_XXX -> N`` mappings found.
        - **block_info** — summary of what was found inside matching blocks.
    """
    config_h_path = os.path.join(config_h_dir, "config.h")
    if not os.path.isfile(config_h_path):
        return {}

    try:
        with open(config_h_path, "r", encoding="utf-8", errors="replace") as f:
            content = f.read()
    except OSError:
        return {}

    lines = content.split("\n")

    # ------------------------------------------------------------------
    # Step 1 — BOARD_XXX name → number mappings
    # ------------------------------------------------------------------
    board_map: Dict[str, int] = {}
    for m in _re.finditer(r'#define\s+(BOARD_\w+)\s+(\d+)', content):
        board_map[m.group(1)] = int(m.group(2))

    # ------------------------------------------------------------------
    # Step 2 — Determine active __BOARD value
    # ------------------------------------------------------------------
    defined_set: Set[str] = set()
    active_board_value: Optional[int] = None

    for d in defines:
        clean = d
        if clean.startswith("-D"):
            clean = clean[2:]
        if "=" in clean:
            name, val = clean.split("=", 1)
            defined_set.add(name)
            if name == "__BOARD":
                try:
                    active_board_value = int(val)
                except ValueError:
                    pass
        else:
            defined_set.add(clean)

    # Fallback: ``#ifndef __BOARD`` / ``#define __BOARD BOARD_XXX`` default
    if active_board_value is None:
        m = _re.search(
            r'#ifndef\s+__BOARD\s*\n\s*#define\s+__BOARD\s+(BOARD_\w+)',
            content,
        )
        if m:
            default_board = m.group(1)
            if default_board in board_map:
                active_board_value = board_map[default_board]

    if active_board_value is None:
        return {"error": "Cannot determine active __BOARD value"}

    # Resolve number back to canonical name
    active_board_name: Optional[str] = None
    for name, val in board_map.items():
        if val == active_board_value:
            active_board_name = name
            break

    # ------------------------------------------------------------------
    # Step 3 — Preprocessor state machine
    # ------------------------------------------------------------------
    # Stack tracks nesting of #if / #elif / #else / #endif.
    # Each entry::
    #   {"active": bool,        # Is this branch currently compiling?
    #    "branch_taken": bool,  # Was any branch taken at this level?
    #    "is_board_level": bool}# Does this level's condition mention __BOARD?
    #
    # ``in_board_scope`` is True whenever ANY stack entry has
    # ``is_board_level == True`` (i.e. we are inside a board-specific block).

    stack: List[Dict[str, Any]] = []

    # Collected data from matching board blocks
    must_define: List[tuple] = []  # (flag, reason)
    block_enabled_1: List[str] = []
    block_enabled_0: List[str] = []
    block_commented_flag_only: List[tuple] = []  # (flag, lineno)

    def _in_board_scope() -> bool:
        return any(level["is_board_level"] for level in stack)

    def _is_active() -> bool:
        """True when all nesting levels are in an active (taken) branch."""
        return all(level["active"] for level in stack) if stack else False

    # ------------------------------------------------------------------
    # Parent-activity helpers
    #
    # There are two distinct parent checks needed:
    #
    # 1. When about to **push** a new ``#if`` / ``#ifdef`` / ``#ifndef``
    #    level — the “parent” is the current top of stack.  If the
    #    current top has ``active=False`` (e.g. inside an include guard
    #    ``#ifndef __CONFIG_H__`` whose condition was not met) then the
    #    new level must also be inactive.
    #
    # 2. When evaluating ``#elif`` / ``#else`` inside an existing
    #    ``#if`` / ``#elif`` / ``#else`` chain — the “parent” is the
    #    level ONE ABOVE the chain (i.e. ``stack[-2]``).  The chain’s
    #    own entries (``stack[-1]``) are siblings, not parents.

    def _parent_before_push() -> bool:
        """Active status of the level that will enclose a new ``#if``."""
        return stack[-1]["active"] if stack else True

    def _parent_of_chain() -> bool:
        """Active status of the level *above* the current ``#if``/``#elif``
        chain (used for ``#elif`` and ``#else``)."""
        return stack[-2]["active"] if len(stack) >= 2 else True

    for i, line in enumerate(lines):
        stripped = line.strip()

        # --- Content extraction (only inside active board-scope blocks) ---
        if _in_board_scope() and _is_active():

            # Commented-out define: ``// #define __XXX 1``
            cm = _re.match(r'//{1,2}\s*#define\s+(__[A-Z][A-Z0-9_]+)\s+(\d+)',
                           stripped)
            if cm:
                flag, val = cm.group(1), int(cm.group(2))
                if flag not in defined_set and val == 1:
                    block_commented_flag_only.append((flag, i))
                continue

            # Direct define: ``#define __XXX N``
            dm = _re.match(r'#define\s+(__[A-Z][A-Z0-9_]+)\s+(\d+)',
                           stripped)
            if dm:
                flag, val = dm.group(1), int(dm.group(2))
                if flag not in defined_set and val == 1:
                    must_define.append((
                        flag,
                        f"#define {flag} 1 in board block (line {i + 1})",
                    ))
                if val == 1:
                    block_enabled_1.append(flag)
                else:
                    block_enabled_0.append(flag)
                continue

            # #ifndef __XXX guard — look ahead for the matching #define
            ifm = _re.match(r'#ifndef\s+(__[A-Z][A-Z0-9_]+)', stripped)
            if ifm:
                guarded_flag = ifm.group(1)
                if guarded_flag not in defined_set:
                    # Peek next few lines for ``#define same_flag N``
                    for j in range(i + 1, min(i + 5, len(lines))):
                        gm = _re.match(
                            r'\s*#define\s+' + _re.escape(guarded_flag)
                            + r'\s+(\d+)',
                            lines[j],
                        )
                        if gm:
                            gval = int(gm.group(1))
                            if gval == 1:
                                must_define.append((
                                    guarded_flag,
                                    f"#ifndef {guarded_flag} + #define "
                                    f"{guarded_flag} 1 in board block"
                                    f" (line {i + 1})",
                                ))
                            break

        # --- Preprocessor directive state machine ---
        # Track ALL conditional directives (#if, #ifdef, #ifndef, #elif,
        # #else, #endif) so the #endif count stays correct.  Previously
        # #ifndef was not tracked, causing its #endif to wrongly pop a
        # board-level entry and corrupt the stack.

        if stripped.startswith("#if "):
            # #if <condition>
            condition = stripped[4:].strip()
            parent_active_val = _parent_before_push()

            if not parent_active_val:
                matched = False
            else:
                matched = _evaluate_board_condition(
                    condition, active_board_value, board_map,
                )

            # Inside board scope: be permissive for non-board conditions
            if _in_board_scope() and not matched and "__BOARD" not in condition:
                matched = True

            is_board_level = "__BOARD" in condition
            stack.append({
                "active": matched,
                "branch_taken": matched,
                "is_board_level": is_board_level,
            })

        elif stripped.startswith("#ifdef") or stripped.startswith("#ifndef"):
            # #ifdef SYMBOL / #ifndef SYMBOL  — push a level so the matching
            # #endif can pop it without corrupting the parent block.
            parent_active_val = _parent_before_push()
            if not parent_active_val:
                matched = False
            else:
                parts = stripped.split(None, 1)
                symbol = parts[1].strip() if len(parts) >= 2 else ""
                if stripped.startswith("#ifndef"):
                    matched = symbol not in defined_set
                else:  # #ifdef
                    matched = symbol in defined_set
            # Inside board scope: be permissive (cannot evaluate all symbols
            # that may come from external -D flags or scattered comments)
            if _in_board_scope() and not matched:
                matched = True
            stack.append({
                "active": matched,
                "branch_taken": matched,
                "is_board_level": False,
            })

        elif stripped.startswith("#elif"):
            condition = stripped[5:].strip()
            if stack:
                level = stack[-1]
                if not level["branch_taken"] and _parent_of_chain():
                    matched = _evaluate_board_condition(
                        condition, active_board_value, board_map,
                    )
                    # Permissive for non-board conditions inside board scope
                    if (_in_board_scope() and not matched
                            and "__BOARD" not in condition):
                        matched = True
                    level["active"] = matched
                    level["branch_taken"] = matched
                else:
                    level["active"] = False

                if "__BOARD" in condition:
                    level["is_board_level"] = True

        elif stripped.startswith("#else"):
            if stack:
                level = stack[-1]
                if not level["branch_taken"] and _parent_of_chain():
                    level["active"] = True
                    level["branch_taken"] = True
                else:
                    level["active"] = False

        elif stripped.startswith("#endif"):
            if stack:
                stack.pop()

    # ------------------------------------------------------------------
    # Step 4 — Flags referenced in preprocessor conditions but never
    #          defined anywhere in config.h
    # ------------------------------------------------------------------
    # Collect all ``#define __XXX`` and ``#ifndef __XXX`` (these are the
    # definitions / guards).
    all_defined: Set[str] = set()
    for m in _re.finditer(r'#define\s+(__[A-Z][A-Z0-9_]+)\s', content):
        all_defined.add(m.group(1))
    for m in _re.finditer(r'#ifndef\s+(__[A-Z][A-Z0-9_]+)', content):
        all_defined.add(m.group(1))

    # Collect ``__XXX`` in ``#if``, ``#elif``, ``#ifdef`` conditions
    # (skip ``#ifndef`` — already counted as definitions).
    all_referenced: Set[str] = set()
    for m in _re.finditer(
            r'#(?:if|elif|ifdef)\b([^#]*?)(__[A-Z][A-Z0-9_]+)',
            content,
    ):
        flag = m.group(2)
        if flag not in all_defined and flag not in defined_set:
            all_referenced.add(flag)

    never_defined = sorted(all_referenced)

    # ------------------------------------------------------------------
    # Step 5 — Build final result
    # ------------------------------------------------------------------

    # Add commented-out flags as suggestions
    for flag, lineno in block_commented_flag_only:
        must_define.append((
            flag,
            f"Commented out in board block (line {lineno + 1}) — "
            f"was probably a Keil IDE -D define",
        ))

    # Deduplicate must_define (keep first occurrence)
    seen: Set[str] = set()
    deduped_must: List[tuple] = []
    for item in must_define:
        if item[0] not in seen:
            seen.add(item[0])
            deduped_must.append(item)

    return {
        "board_name": active_board_name or f"UNKNOWN({active_board_value})",
        "board_value": active_board_value,
        "must_define": sorted(deduped_must, key=lambda x: x[0]),
        "never_defined": [(f, "Referenced in preprocessor condition "
                              "but never #defined in config.h")
                          for f in never_defined],
        "board_map": board_map,
        "board_count": len(board_map),
        "block_info": {
            "enabled_1_count": len(set(block_enabled_1)),
            "enabled_1": sorted(set(block_enabled_1)),
            "enabled_0_count": len(set(block_enabled_0)),
            "enabled_0": sorted(set(block_enabled_0)),
            "commented_count": len(block_commented_flag_only),
        },
    }