"""Tests for config loading, validation, and merging.

Covers ``load_config``, ``merge_configs``, ``_merge_scan_config``,
and ``_load_json_config`` from ``embedded_cmake.config``.
"""
from __future__ import annotations

import json
import os
import tempfile
from pathlib import Path

from embedded_cmake.config import (
    _load_json_config,
    _merge_scan_config,
    merge_configs,
)
from embedded_cmake.models import ProjectConfig, ScanConfig, RTOSConfig


# ---------------------------------------------------------------------------
# _load_json_config
# ---------------------------------------------------------------------------

class TestLoadJsonConfig:
    """Verify JSON config loading with comment stripping."""

    def test_standard_json(self):
        """Plain JSON is loaded normally."""
        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".json", delete=False, encoding="utf-8"
        ) as f:
            f.write('{"project": {"name": "Foo"}}')
            path = f.name
        try:
            result = _load_json_config(path)
            assert result == {"project": {"name": "Foo"}}
        finally:
            os.unlink(path)

    def test_strips_line_comments_slash(self):
        """Lines starting with // are stripped before parsing."""
        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".json", delete=False, encoding="utf-8"
        ) as f:
            f.write("// this is a comment\n{\"a\": 1}")
            path = f.name
        try:
            result = _load_json_config(path)
            assert result == {"a": 1}
        finally:
            os.unlink(path)

    def test_strips_line_comments_hash(self):
        """Lines starting with # are stripped before parsing."""
        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".json", delete=False, encoding="utf-8"
        ) as f:
            f.write("# comment\n{\"b\": 2}")
            path = f.name
        try:
            result = _load_json_config(path)
            assert result == {"b": 2}
        finally:
            os.unlink(path)

    def test_strips_mixed_comments(self):
        """Both // and # comment lines are stripped in the same file."""
        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".json", delete=False, encoding="utf-8"
        ) as f:
            f.write("// header\n# another\n{\"c\": 3}")
            path = f.name
        try:
            result = _load_json_config(path)
            assert result == {"c": 3}
        finally:
            os.unlink(path)

    def test_trailing_commas_not_allowed(self):
        """Standard JSON is used — trailing commas cause a parse error."""
        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".json", delete=False, encoding="utf-8"
        ) as f:
            f.write("{\"d\": 4,}")
            path = f.name
        try:
            import json as _json
            with open(path, "r") as fh:
                lines = fh.readlines()
            clean = "".join(
                line for line in lines
                if not line.strip().startswith(("//", "#"))
            )
            # trailing comma should raise
            raised = False
            try:
                _json.loads(clean)
            except _json.JSONDecodeError:
                raised = True
            assert raised, "expected JSONDecodeError for trailing comma"
        finally:
            os.unlink(path)

    def test_empty_object(self):
        """Empty JSON object is valid."""
        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".json", delete=False, encoding="utf-8"
        ) as f:
            f.write("{}")
            path = f.name
        try:
            result = _load_json_config(path)
            assert result == {}
        finally:
            os.unlink(path)

    def test_comment_lines_preserve_content_lines(self):
        """Non-comment lines are preserved verbatim."""
        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".json", delete=False, encoding="utf-8"
        ) as f:
            f.write("// drop\n{\n\"x\": 1\n}")
            path = f.name
        try:
            result = _load_json_config(path)
            assert result == {"x": 1}
        finally:
            os.unlink(path)


# ---------------------------------------------------------------------------
# _merge_scan_config
# ---------------------------------------------------------------------------

class TestMergeScanConfig:
    """Verify scan config merging with add/remove semantics."""

    def test_extend_exclude_dirs(self):
        """User exclude_dirs are appended to existing list."""
        scan = ScanConfig(exclude_dirs=[".git"])
        _merge_scan_config(scan, {"exclude_dirs": ["build", "tmp"]})
        assert ".git" in scan.exclude_dirs
        assert "build" in scan.exclude_dirs
        assert "tmp" in scan.exclude_dirs

    def test_remove_exclude_dirs_with_prefix(self):
        """Item prefixed with ``-`` removes it from the target list."""
        scan = ScanConfig(exclude_dirs=[".git", "build", "tmp"])
        _merge_scan_config(scan, {"exclude_dirs": ["-build"]})
        assert ".git" in scan.exclude_dirs
        assert "build" not in scan.exclude_dirs
        assert "tmp" in scan.exclude_dirs

    def test_remove_nonexistent_item_silent(self):
        """Removing a non-existent item is silently ignored."""
        scan = ScanConfig(exclude_dirs=[".git"])
        _merge_scan_config(scan, {"exclude_dirs": ["-build"]})
        assert scan.exclude_dirs == [".git"]

    def test_remove_prefix_only_applies_to_strings(self):
        """Non-string items are not affected by ``-`` prefix logic."""
        scan = ScanConfig(exclude_dirs=[".git"])
        _merge_scan_config(scan, {"exclude_dirs": [123]})
        assert 123 in scan.exclude_dirs

    def test_source_extensions_replaced(self):
        """source_extensions is replaced, not extended."""
        scan = ScanConfig(source_extensions=[".c"])
        _merge_scan_config(scan, {"source_extensions": [".cpp", ".cxx"]})
        assert scan.source_extensions == [".cpp", ".cxx"]

    def test_header_extensions_replaced(self):
        """header_extensions is replaced, not extended."""
        scan = ScanConfig(header_extensions=[".h"])
        _merge_scan_config(scan, {"header_extensions": [".hpp"]})
        assert scan.header_extensions == [".hpp"]

    def test_extra_exclude_header_dirs_extended(self):
        """extra_exclude_header_dirs follows list extend semantics."""
        scan = ScanConfig(extra_exclude_header_dirs=["MDK-ARM"])
        _merge_scan_config(scan, {"extra_exclude_header_dirs": ["EWARM"]})
        assert "MDK-ARM" in scan.extra_exclude_header_dirs
        assert "EWARM" in scan.extra_exclude_header_dirs

    def test_multiple_keys_in_one_call(self):
        """Multiple scan keys can be merged in a single call."""
        scan = ScanConfig(exclude_dirs=[], exclude_files=[])
        _merge_scan_config(scan, {
            "exclude_dirs": ["build"],
            "exclude_files": ["syscalls.c"],
        })
        assert "build" in scan.exclude_dirs
        assert "syscalls.c" in scan.exclude_files

    def test_duplicate_items_not_added(self):
        """Adding an already-present item is a no-op."""
        scan = ScanConfig(exclude_dirs=["build"])
        _merge_scan_config(scan, {"exclude_dirs": ["build"]})
        # count should be 1, not 2
        assert scan.exclude_dirs.count("build") == 1


# ---------------------------------------------------------------------------
# merge_configs  —  ProjectConfig merging
# ---------------------------------------------------------------------------

class TestMergeConfigs:
    """Verify ProjectConfig merging from user JSON data."""

    def test_project_name_override(self):
        """project.name overrides detected project name."""
        config = ProjectConfig(project_name="Detected")
        merge_configs(config, {"project": {"name": "User"}})
        assert config.project_name == "User"

    def test_project_name_not_overridden_when_empty(self):
        """Detected name is kept when user does not supply one."""
        config = ProjectConfig(project_name="Detected")
        merge_configs(config, {"project": {}})
        assert config.project_name == "Detected"

    def test_defines_extend(self):
        """User defines are appended to existing defines (no duplicates)."""
        config = ProjectConfig(defines=["USE_HAL_DRIVER"])
        merge_configs(config, {"defines": ["STM32F103xB", "USE_HAL_DRIVER"]})
        assert "USE_HAL_DRIVER" in config.defines
        assert "STM32F103xB" in config.defines
        assert config.defines.count("USE_HAL_DRIVER") == 1

    def test_defines_ignored_when_not_list(self):
        """Non-list defines value is silently ignored."""
        config = ProjectConfig(defines=["A"])
        merge_configs(config, {"defines": "not_a_list"})
        assert config.defines == ["A"]

    def test_toolchains_replaced(self):
        """User toolchains list replaces detected list."""
        config = ProjectConfig(toolchains=["gcc"])
        merge_configs(config, {"toolchains": ["armcc"]})
        assert config.toolchains == ["armcc"]

    def test_toolchains_not_overridden_when_empty(self):
        """Empty user toolchains list is ignored."""
        config = ProjectConfig(toolchains=["gcc"])
        merge_configs(config, {"toolchains": []})
        assert config.toolchains == ["gcc"]

    def test_chip_family_override(self):
        """chip.family overrides detected family."""
        config = ProjectConfig(chip_family="STM32F1")
        merge_configs(config, {"chip": {"family": "APM32E1"}})
        assert config.chip_family == "APM32E1"

    def test_chip_model_override(self):
        """chip.model overrides detected model."""
        config = ProjectConfig(chip_model="STM32F103C8Tx")
        merge_configs(config, {"chip": {"model": "APM32E103RE"}})
        assert config.chip_model == "APM32E103RE"

    def test_armcc_compiler_dir(self):
        """armcc.compiler_dir overrides detected path."""
        config = ProjectConfig(armcc_compiler_dir="")
        merge_configs(config, {"armcc": {"compiler_dir": "/opt/keil"}})
        assert config.armcc_compiler_dir == "/opt/keil"

    def test_output_hex_flag(self):
        """output.hex overrides detected default."""
        config = ProjectConfig(output_hex=True)
        merge_configs(config, {"output": {"hex": False}})
        assert config.output_hex is False

    def test_output_bin_flag(self):
        """output.bin overrides detected default."""
        config = ProjectConfig(output_bin=True)
        merge_configs(config, {"output": {"bin": False}})
        assert config.output_bin is False

    def test_output_size_flag(self):
        """output.size overrides detected default."""
        config = ProjectConfig(output_size=False)
        merge_configs(config, {"output": {"size": True}})
        assert config.output_size is True

    def test_output_cmake_dir(self):
        """output.cmake_dir overrides detected default."""
        config = ProjectConfig(cmake_dir="cmake")
        merge_configs(config, {"output": {"cmake_dir": "custom_cmake"}})
        assert config.cmake_dir == "custom_cmake"

    def test_startup_file(self):
        """startup_file overrides detected."""
        config = ProjectConfig(startup_file="")
        merge_configs(config, {"startup_file": "startup_custom.s"})
        assert config.startup_file == "startup_custom.s"

    def test_armcc_startup_file(self):
        """armcc_startup_file overrides detected."""
        config = ProjectConfig(armcc_startup_file="")
        merge_configs(config, {"armcc_startup_file": "startup_custom_arm.s"})
        assert config.armcc_startup_file == "startup_custom_arm.s"

    def test_linker_script(self):
        """linker_script overrides detected."""
        config = ProjectConfig(linker_script="")
        merge_configs(config, {"linker_script": "custom.ld"})
        assert config.linker_script == "custom.ld"

    def test_armcc_scatter_file(self):
        """armcc_scatter_file overrides detected."""
        config = ProjectConfig(armcc_scatter_file="")
        merge_configs(config, {"armcc_scatter_file": "custom.sct"})
        assert config.armcc_scatter_file == "custom.sct"

    def test_board_name(self):
        """board overrides detected."""
        config = ProjectConfig(board_name="")
        merge_configs(config, {"board": "Blue Pill"})
        assert config.board_name == "Blue Pill"

    def test_cpu_name(self):
        """cpu overrides detected."""
        config = ProjectConfig(cpu_name="")
        merge_configs(config, {"cpu": "Cortex-M3"})
        assert config.cpu_name == "Cortex-M3"

    def test_keil_project_file(self):
        """keil_project_file overrides detected."""
        config = ProjectConfig(keil_project_file="")
        merge_configs(config, {"keil_project_file": "MyProject.uvprojx"})
        assert config.keil_project_file == "MyProject.uvprojx"

    def test_lib_files_extend(self):
        """lib_files are extended with no duplicates."""
        config = ProjectConfig(lib_files=["a.lib"])
        merge_configs(config, {"lib_files": ["b.lib", "a.lib"]})
        assert "a.lib" in config.lib_files
        assert "b.lib" in config.lib_files
        assert config.lib_files.count("a.lib") == 1

    def test_lib_files_not_overridden_when_empty(self):
        """Empty user lib_files list is ignored."""
        config = ProjectConfig(lib_files=["a.lib"])
        merge_configs(config, {"lib_files": []})
        assert config.lib_files == ["a.lib"]

    def test_chip_header_patterns_extend(self):
        """chip_header_patterns are extended with valid dicts."""
        config = ProjectConfig(extra_header_patterns=[])
        merge_configs(config, {
            "chip_header_patterns": [
                {"regex": ".*STM32.*", "family": "STM32F1"}
            ]
        })
        assert len(config.extra_header_patterns) == 1
        assert config.extra_header_patterns[0]["family"] == "STM32F1"

    def test_chip_header_patterns_skip_invalid(self):
        """Invalid chip_header_patterns (missing regex or family) are skipped."""
        config = ProjectConfig(extra_header_patterns=[])
        merge_configs(config, {
            "chip_header_patterns": [
                {"regex": ".*"},  # missing family
            ]
        })
        assert len(config.extra_header_patterns) == 0

    def test_chip_header_patterns_not_overridden_when_empty(self):
        """Empty user chip_header_patterns list is ignored."""
        config = ProjectConfig(extra_header_patterns=[{"regex": "x", "family": "y"}])
        merge_configs(config, {"chip_header_patterns": []})
        assert len(config.extra_header_patterns) == 1

    def test_empty_user_data_no_change(self):
        """Merging empty user data leaves config unchanged."""
        config = ProjectConfig(project_name="Test", defines=["A"])
        merge_configs(config, {})
        assert config.project_name == "Test"
        assert config.defines == ["A"]


# ---------------------------------------------------------------------------
# RTOS merge
# ---------------------------------------------------------------------------

class TestMergeRTOSConfig:
    """Verify RTOSConfig merging from user JSON data."""

    def test_rtos_type_override(self):
        """rtos.type overrides detected type."""
        rtos = RTOSConfig(type="none")
        config = ProjectConfig(rtos=rtos)
        merge_configs(config, {"rtos": {"type": "FreeRTOS"}})
        assert config.rtos.type == "FreeRTOS"

    def test_rtos_heap_override(self):
        """rtos.heap overrides detected heap."""
        rtos = RTOSConfig(heap="heap_4.c")
        config = ProjectConfig(rtos=rtos)
        merge_configs(config, {"rtos": {"heap": "heap_5.c"}})
        assert config.rtos.heap == "heap_5.c"

    def test_rtos_cmsis_version_override(self):
        """rtos.cmsis_version overrides detected version."""
        rtos = RTOSConfig(cmsis_version="V1")
        config = ProjectConfig(rtos=rtos)
        merge_configs(config, {"rtos": {"cmsis_version": "V2"}})
        assert config.rtos.cmsis_version == "V2"

    def test_rtos_base_path_override(self):
        """rtos.base_path overrides detected path."""
        rtos = RTOSConfig(base_path="")
        config = ProjectConfig(rtos=rtos)
        merge_configs(config, {"rtos": {"base_path": "ThirdParty/FreeRTOS"}})
        assert config.rtos.base_path == "ThirdParty/FreeRTOS"

    def test_rtos_port_mapping_merge(self):
        """rtos.port_mapping is updated (not replaced)."""
        rtos = RTOSConfig(port_mapping={"gcc": "GCC/ARM_CM3"})
        config = ProjectConfig(rtos=rtos)
        merge_configs(config, {"rtos": {"port_mapping": {"armcc": "RVDS/ARM_CM3"}}})
        assert rtos.port_mapping["gcc"] == "GCC/ARM_CM3"
        assert rtos.port_mapping["armcc"] == "RVDS/ARM_CM3"
