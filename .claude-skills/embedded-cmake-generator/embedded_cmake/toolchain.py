"""Toolchain configuration loader and flag resolver.

Loads toolchain definitions from JSON files and resolves CPU/FPU flags
for specific chip configurations.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Dict, List, Optional

from .models import ToolchainConfig, PostBuildConfig, PostBuildStep, CpuInfo


def _default_data_dir() -> Path:
    """Get the default toolchain data directory relative to this module."""
    return Path(__file__).resolve().parent / "data" / "toolchains"


def _parse_post_build(data: Optional[dict]) -> PostBuildConfig:
    """Parse post-build configuration from JSON dict."""
    if not data:
        return PostBuildConfig()
    return PostBuildConfig(
        hex=_parse_post_build_step(data.get("hex")),
        bin=_parse_post_build_step(data.get("bin")),
        size=_parse_post_build_step(data.get("size")),
    )


def _parse_post_build_step(data: Optional[dict]) -> Optional[PostBuildStep]:
    """Parse a single post-build step."""
    if not data:
        return None
    return PostBuildStep(
        command=data.get("command", []),
        comment=data.get("comment", ""),
    )


def _parse_toolchain_json(data: dict) -> ToolchainConfig:
    """Parse a toolchain JSON dict into a ToolchainConfig."""
    post_build = _parse_post_build(data.get("post_build"))
    return ToolchainConfig(
        name=data.get("name", ""),
        toolchain_id=data.get("toolchain_id", ""),
        cmake_compiler_id=data.get("cmake_compiler_id", ""),
        compiler_method=data.get("compiler_method", "prefix"),
        compiler_prefix=data.get("compiler_prefix", ""),
        tools=data.get("tools", {}),
        executable_suffix=data.get("executable_suffix", ".elf"),
        try_compile_target_type=data.get("try_compile_target_type", "STATIC_LIBRARY"),
        cpu_flags=data.get("cpu_flags", {}),
        fpu_flags=data.get("fpu_flags", {}),
        flags=data.get("flags", {}),
        link_libraries=data.get("link_libraries", []),
        linker_script_option_template=data.get("linker_script_option_template", "-T{path}"),
        post_build=post_build,
        target_properties=data.get("target_properties", {}),
    )


class ToolchainRegistry:
    """Registry of toolchain definitions loaded from JSON files.

    Usage::

        reg = ToolchainRegistry()
        reg.load_builtin()
        gcc = reg.get("gcc")
        flags = gcc.resolve_cpu_flags(CpuInfo(core="Cortex-M3", fpu="none"))
    """

    def __init__(self, search_paths: Optional[List[Path]] = None):
        self._toolchains: Dict[str, ToolchainConfig] = {}
        self._search_paths: List[Path] = list(search_paths or [])

    def add_search_path(self, path: Path) -> None:
        """Add a search path for JSON toolchain files."""
        if path not in self._search_paths:
            self._search_paths.append(path)

    def load_builtin(self) -> int:
        """Load all built-in toolchain definitions. Returns count of files loaded."""
        self._search_paths.append(_default_data_dir())
        return self._load_from_paths()

    def load_path(self, path: Path) -> int:
        """Load toolchain JSON files from a specific directory."""
        count = 0
        if path.is_dir():
            for f in sorted(path.glob("*.json")):
                self._load_file(f)
                count += 1
        return count

    def _load_from_paths(self) -> int:
        """Load from all registered search paths."""
        count = 0
        for path in self._search_paths:
            count += self.load_path(path)
        return count

    def _load_file(self, filepath: Path) -> Optional[ToolchainConfig]:
        """Load a single toolchain JSON file."""
        try:
            with open(filepath, "r", encoding="utf-8") as f:
                data = json.load(f)
            tc = _parse_toolchain_json(data)
            self.register(tc)
            return tc
        except (json.JSONDecodeError, KeyError, TypeError) as e:
            import sys
            print(f"[WARN] Failed to load toolchain file {filepath}: {e}", file=sys.stderr)
            return None

    def register(self, tc: ToolchainConfig) -> None:
        """Register a ToolchainConfig."""
        self._toolchains[tc.toolchain_id] = tc

    def get(self, toolchain_id: str) -> Optional[ToolchainConfig]:
        """Get a toolchain by ID (e.g. 'gcc', 'armcc')."""
        return self._toolchains.get(toolchain_id)

    def list_ids(self) -> List[str]:
        """List all registered toolchain IDs."""
        return sorted(self._toolchains.keys())

    def resolve_flags(self, toolchain_id: str, cpu: CpuInfo) -> str:
        """Resolve CPU + FPU flags for a specific toolchain and CPU."""
        tc = self.get(toolchain_id)
        if not tc:
            return ""
        return tc.resolve_cpu_flags(cpu)

    def get_linker_option(self, toolchain_id: str, path: str) -> str:
        """Get the linker script option for a toolchain and script path."""
        tc = self.get(toolchain_id)
        if not tc:
            return f"-T{path}"
        return tc.linker_script_option_template.format(path=path)

    def __len__(self) -> int:
        return len(self._toolchains)

    def __contains__(self, toolchain_id: str) -> bool:
        return toolchain_id in self._toolchains
