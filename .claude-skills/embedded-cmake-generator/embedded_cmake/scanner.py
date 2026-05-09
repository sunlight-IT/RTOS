"""Configurable source and header file scanner.

Scans project directories for C/C++/ASM source files and header directories,
with configurable include/exclude rules from ScanConfig.
"""

from __future__ import annotations

import os
from pathlib import Path
from typing import List, Set

from .models import ScanConfig, ScanResult


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


def _match_simple_pattern(name: str, pattern: str) -> bool:
    """Simple glob-style pattern matching (supports * and ?).

    Avoids importing fnmatch for this simple case; we only need
    prefix/suffix/contains matching for directory and file names.
    """
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


def scan_sources(project_dir: str, config: Optional[ScanConfig] = None) -> ScanResult:
    """Scan project directory for source files.

    Args:
        project_dir: Root project directory path.
        config: Scan configuration. Uses defaults if not provided.

    Returns:
        ScanResult with categorized file lists.
    """
    if config is None:
        config = ScanConfig()

    c_sources: List[str] = []
    cpp_sources: List[str] = []
    asm_sources: List[str] = []

    source_exts = set(config.source_extensions)
    cpp_exts = {".cpp", ".cxx", ".cc"}
    asm_exts = {".s", ".S"}

    for root, dirs, files in os.walk(project_dir):
        # Apply directory exclusions
        dirs[:] = [d for d in dirs if not _should_exclude_dir_name(d, config)]

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
    )


def _should_exclude_dir_name(dirname: str, config: ScanConfig) -> bool:
    """Check if a directory name (single component) should be excluded."""
    if dirname in config.exclude_dirs:
        return True
    for pattern in config.exclude_dir_patterns:
        if _match_simple_pattern(dirname, pattern):
            return True
    return False


def scan_headers(
    project_dir: str,
    config: Optional[ScanConfig] = None,
    extra_exclude_subdirs: Optional[List[str]] = None,
) -> List[str]:
    """Scan project directory for directories containing header files.

    Args:
        project_dir: Root project directory path.
        config: Scan configuration. Uses defaults if not provided.
        extra_exclude_subdirs: Additional subdir names to exclude from header paths.

    Returns:
        Sorted list of directories containing header files.
    """
    if config is None:
        config = ScanConfig()

    exclude_subdirs = list(config.extra_exclude_header_dirs)
    if extra_exclude_subdirs:
        exclude_subdirs.extend(extra_exclude_subdirs)

    header_dirs: Set[str] = set()
    header_exts = set(config.header_extensions)

    for root, dirs, files in os.walk(project_dir):
        # Apply directory exclusions
        dirs[:] = [d for d in dirs if not _should_exclude_dir_name(d, config)]
        dirs[:] = [d for d in dirs if d not in exclude_subdirs]

        if _should_exclude_path(root, config):
            continue

        for file in files:
            for ext in header_exts:
                if file.endswith(ext):
                    header_dirs.add(os.path.normpath(root))
                    break

    return sorted(header_dirs)
