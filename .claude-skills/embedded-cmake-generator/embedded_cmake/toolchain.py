"""Toolchain configuration loader and flag resolver.

Loads toolchain definitions from JSON files and resolves CPU/FPU flags
for specific chip configurations.
"""

from __future__ import annotations

from pathlib import Path
from typing import Dict, List, Optional

from .json_registry import JsonRegistry
from .models import CpuInfo, PostBuildConfig, PostBuildStep, ToolchainConfig


def _default_data_dir() -> Path:
    """Get the default toolchain data directory relative to this module."""
    return Path(__file__).resolve().parent / "data" / "toolchains"


def _parse_post_build(data: Optional[dict]) -> PostBuildConfig:
    if not data:
        return PostBuildConfig()
    return PostBuildConfig(
        hex=_parse_post_build_step(data.get("hex")),
        bin=_parse_post_build_step(data.get("bin")),
        size=_parse_post_build_step(data.get("size")),
    )


def _parse_post_build_step(data: Optional[dict]) -> Optional[PostBuildStep]:
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
        link_library_suffix=data.get("link_library_suffix", ".a"),
        static_library_suffix=data.get("static_library_suffix", ".a"),
        try_compile_target_type=data.get("try_compile_target_type", "STATIC_LIBRARY"),
        cpu_flags=data.get("cpu_flags", {}),
        fpu_flags=data.get("fpu_flags", {}),
        arch_mappings=data.get("arch_mappings", {}),
        flags=data.get("flags", {}),
        link_libraries=data.get("link_libraries", []),
        linker_script_option_template=data.get("linker_script_option_template", "-T{path}"),
        post_build=post_build,
        target_properties=data.get("target_properties", {}),
    )


class ToolchainRegistry(JsonRegistry[ToolchainConfig]):
    """Registry of toolchain definitions loaded from JSON files.

    Usage::

        reg = ToolchainRegistry()
        reg.add_search_path(Path("path/to/toolchains"))
        reg.load_builtin()
        gcc = reg.get("gcc")
        flags = gcc.resolve_cpu_flags(CpuInfo(core="Cortex-M3", fpu="none"))
    """

    def __init__(self, search_paths: Optional[List[Path]] = None):
        super().__init__(search_paths)

    # -- JsonRegistry hooks -----------------------------------------------

    def _parse_item(self, data: dict) -> ToolchainConfig:
        return _parse_toolchain_json(data)

    def _register_item(self, item: ToolchainConfig) -> None:
        self._items[item.toolchain_id] = item

    def load_builtin(self) -> int:
        """Load built-in toolchain definitions."""
        self.add_search_path(_default_data_dir())
        return self._load_from_paths()

    # -- lookup API -------------------------------------------------------

    def get(self, toolchain_id: str) -> Optional[ToolchainConfig]:
        """Get a toolchain by ID (e.g. 'gcc', 'armcc')."""
        return self._items.get(toolchain_id)

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
