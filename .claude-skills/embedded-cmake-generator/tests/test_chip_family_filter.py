"""Tests for chip-family-aware directory filtering in scanner.

Covers ``_get_chip_family_prefix``, ``_is_chip_family_dir``,
and ``_build_chip_family_filter`` from ``embedded_cmake.scanner``.
"""
from __future__ import annotations

import os
import tempfile
from typing import List, Optional

from embedded_cmake.scanner import (
    _build_chip_family_filter,
    _get_chip_family_prefix,
    _is_chip_family_dir,
)


# ---------------------------------------------------------------------------
# _get_chip_family_prefix
# ---------------------------------------------------------------------------

class TestGetChipFamilyPrefix:
    """Extract base prefix from chip family name (stripping trailing digits)."""

    def test_stm32l1_becomes_stm32l(self):
        """STM32L1 -> stm32l"""
        assert _get_chip_family_prefix("STM32L1") == "stm32l"

    def test_stm32f1_becomes_stm32f(self):
        """STM32F1 -> stm32f"""
        assert _get_chip_family_prefix("STM32F1") == "stm32f"

    def test_apm32e1_becomes_apm32e(self):
        """APM32E1 -> apm32e"""
        assert _get_chip_family_prefix("APM32E1") == "apm32e"

    def test_apm32f1_becomes_apm32f(self):
        """APM32F1 -> apm32f"""
        assert _get_chip_family_prefix("APM32F1") == "apm32f"

    def test_mk64f_becomes_mk64f(self):
        """MK64F -> mk64f (only trailing digits stripped; 'F' blocks strip)"""
        assert _get_chip_family_prefix("MK64F") == "mk64f"

    def test_lowercase_input(self):
        """Already lowercase input stays lowercase with digits stripped."""
        assert _get_chip_family_prefix("stm32l1") == "stm32l"

    def test_stm32_becomes_stm(self):
        """STM32 -> stm (rstrip removes all trailing digit chars, including '32')."""
        assert _get_chip_family_prefix("STM32") == "stm"

    def test_all_digits_name(self):
        """Name that is only digits becomes empty string."""
        assert _get_chip_family_prefix("12345") == ""


# ---------------------------------------------------------------------------
# _is_chip_family_dir
# ---------------------------------------------------------------------------

class TestIsChipFamilyDir:
    """Heuristic: uppercase-start + contains-digit = chip family dir."""

    def test_stm32l_is_chip_family(self):
        """STM32L starts with upper and has digit."""
        assert _is_chip_family_dir("STM32L") is True

    def test_apm32_is_chip_family(self):
        """APM32 is chip family."""
        assert _is_chip_family_dir("APM32") is True

    def test_mk64f_is_chip_family(self):
        """MK64F is chip family."""
        assert _is_chip_family_dir("MK64F") is True

    def test_lowercase_not_chip_family(self):
        """Lowercase name is not considered chip family."""
        assert _is_chip_family_dir("stm32l") is False

    def test_no_digits_not_chip_family(self):
        """Name without digits is not chip family."""
        assert _is_chip_family_dir("Hardware") is False

    def test_empty_string_not_chip_family(self):
        """Empty string is not chip family."""
        assert _is_chip_family_dir("") is False

    def test_common_dir_not_chip_family(self):
        """Common directory names like Core, Drivers are not chip family."""
        assert _is_chip_family_dir("Core") is False
        assert _is_chip_family_dir("Drivers") is False
        assert _is_chip_family_dir("Middlewares") is False
        assert _is_chip_family_dir("Usr") is False

    def test_numeric_prefix_not_chip_family(self):
        """Name starting with digit is not chip family."""
        assert _is_chip_family_dir("123STM") is False

    def test_single_upper_then_digit(self):
        """Single uppercase letter followed by digit."""
        assert _is_chip_family_dir("A1") is True


# ---------------------------------------------------------------------------
# _build_chip_family_filter  (unit tests — no filesystem)
# ---------------------------------------------------------------------------

class TestBuildChipFamilyFilterUnit:
    """Test filter construction logic that doesn't need real directories."""

    def test_empty_chip_family_returns_none(self):
        """No chip family -> no filtering."""
        assert _build_chip_family_filter("/tmp/proj", "") is None

    def test_filter_is_callable_when_no_search_paths(self):
        """Without matching search dirs, filter is a no-op callable, not None."""
        result = _build_chip_family_filter("/nonexistent/path", "STM32L1")
        assert callable(result), "Should return a callable even with no search paths"
        # Calling the no-op filter should not modify dirs
        dirs = ["APM32", "STM32L"]
        result("/nonexistent/path", dirs)
        assert dirs == ["APM32", "STM32L"], "No-op filter should not modify dirs"


# ---------------------------------------------------------------------------
# _build_chip_family_filter  (integration tests — with temp directories)
# ---------------------------------------------------------------------------

class TestBuildChipFamilyFilterIntegration:
    """Test filter function behavior with real directory structures."""

    def _make_project(self, base: str, family_dirs: List[str],
                     extra_dirs: Optional[List[str]] = None) -> str:
        """Create a temporary project with MCU family directories under Hardware/."""
        source_hw = os.path.join(base, "source", "Hardware")
        os.makedirs(source_hw, exist_ok=True)
        for fd in family_dirs:
            os.makedirs(os.path.join(source_hw, fd), exist_ok=True)
        if extra_dirs:
            for ed in extra_dirs:
                os.makedirs(os.path.join(base, ed), exist_ok=True)
        return base

    def test_filter_excludes_non_matching_mcu_dirs(self):
        """With family=STM32L1, only STM32L* dirs survive in Hardware/."""
        with tempfile.TemporaryDirectory() as tmp:
            self._make_project(tmp, ["APM32", "MK64F", "STM32L", "STM32"])
            filter_fn = _build_chip_family_filter(tmp, "STM32L1")

            assert filter_fn is not None, "Filter should be built when dirs exist"

            source_hw = os.path.normpath(os.path.join(tmp, "source", "Hardware"))
            dirs = ["APM32", "MK64F", "STM32L", "STM32"]
            filter_fn(source_hw, dirs)

            assert "APM32" not in dirs, "APM32 should be filtered out"
            assert "MK64F" not in dirs, "MK64F should be filtered out"
            assert "STM32L" in dirs, "STM32L should be kept (matches prefix)"
            assert "STM32" not in dirs, "STM32 should be filtered out (no 'L')"

    def test_filter_keeps_all_when_single_mcu_dir(self):
        """When only one MCU dir exists, all dirs pass through."""
        with tempfile.TemporaryDirectory() as tmp:
            self._make_project(tmp, ["STM32L"])
            filter_fn = _build_chip_family_filter(tmp, "STM32L1")

            assert filter_fn is not None
            source_hw = os.path.normpath(os.path.join(tmp, "source", "Hardware"))
            dirs = ["STM32L", "Core", "Drivers"]
            filter_fn(source_hw, dirs)

            assert "STM32L" in dirs
            assert "Core" in dirs
            assert "Drivers" in dirs

    def test_filter_does_not_affect_subdirectories(self):
        """Filter only applies at the search path root, not subdirs."""
        with tempfile.TemporaryDirectory() as tmp:
            self._make_project(tmp, ["APM32", "STM32L"])
            filter_fn = _build_chip_family_filter(tmp, "STM32L1")
            assert filter_fn is not None

            # Subdirectory of Hardware/STM32L — should NOT be filtered
            sub_root = os.path.normpath(
                os.path.join(tmp, "source", "Hardware", "STM32L", "SubDir")
            )
            os.makedirs(sub_root, exist_ok=True)
            dirs = ["APM32_Sub", "Something"]
            filter_fn(sub_root, dirs)

            # All subdirectory names pass through unfiltered
            assert "APM32_Sub" in dirs
            assert "Something" in dirs

    def test_filter_no_op_when_root_not_search_path(self):
        """Filter doesn't modify dirs when root is not a configured search path."""
        with tempfile.TemporaryDirectory() as tmp:
            self._make_project(tmp, ["APM32", "STM32L"])
            filter_fn = _build_chip_family_filter(tmp, "STM32L1")
            assert filter_fn is not None

            # Some unrelated directory
            unrelated = os.path.normpath(os.path.join(tmp, "Core", "Src"))
            os.makedirs(unrelated, exist_ok=True)
            dirs = ["APM32", "STM32L"]
            filter_fn(unrelated, dirs)

            assert "APM32" in dirs
            assert "STM32L" in dirs

    def test_scenario_lora_source_tree(self):
        """Simulate LORA's source tree: Hardware/{APM32, STM32L}, Bsp/{APM32, MK64F, STM32L, STM32}."""
        with tempfile.TemporaryDirectory() as tmp:
            # Create LORA-like structure
            for d in ["source/Hardware/APM32",
                       "source/Hardware/STM32L",
                       "source/Bsp/APM32",
                       "source/Bsp/MK64F",
                       "source/Bsp/STM32L",
                       "source/Bsp/STM32"]:
                os.makedirs(os.path.join(tmp, d), exist_ok=True)

            filter_fn = _build_chip_family_filter(tmp, "STM32L1")
            assert filter_fn is not None

            # Test Hardware filtering
            hw_root = os.path.normpath(os.path.join(tmp, "source", "Hardware"))
            hw_dirs = ["APM32", "STM32L"]
            filter_fn(hw_root, hw_dirs)
            assert "APM32" not in hw_dirs
            assert "STM32L" in hw_dirs

            # Test Bsp filtering
            bsp_root = os.path.normpath(os.path.join(tmp, "source", "Bsp"))
            bsp_dirs = ["APM32", "MK64F", "STM32L", "STM32"]
            filter_fn(bsp_root, bsp_dirs)
            assert "APM32" not in bsp_dirs
            assert "MK64F" not in bsp_dirs
            assert "STM32L" in bsp_dirs
            assert "STM32" not in bsp_dirs

    def test_project_dir_without_source_prefix(self):
        """Project where Hardware/ is directly under project root (not source/)."""
        with tempfile.TemporaryDirectory() as tmp:
            for d in ["Hardware/APM32", "Hardware/STM32L",
                       "Bsp/APM32", "Bsp/STM32L"]:
                os.makedirs(os.path.join(tmp, d), exist_ok=True)

            filter_fn = _build_chip_family_filter(tmp, "STM32L1")
            assert filter_fn is not None

            hw_root = os.path.normpath(os.path.join(tmp, "Hardware"))
            hw_dirs = ["APM32", "STM32L"]
            filter_fn(hw_root, hw_dirs)
            assert "APM32" not in hw_dirs
            assert "STM32L" in hw_dirs

    def test_apm32e1_family_prefix(self):
        """APM32E1 family: only dirs starting with 'apm32e' should survive."""
        with tempfile.TemporaryDirectory() as tmp:
            for d in ["Hardware/APM32E", "Hardware/APM32F", "Hardware/APM32"]:
                os.makedirs(os.path.join(tmp, d), exist_ok=True)

            filter_fn = _build_chip_family_filter(tmp, "APM32E1")
            assert filter_fn is not None

            hw_root = os.path.normpath(os.path.join(tmp, "Hardware"))
            hw_dirs = ["APM32E", "APM32F", "APM32"]
            filter_fn(hw_root, hw_dirs)
            assert "APM32E" in hw_dirs
            assert "APM32F" not in hw_dirs
            assert "APM32" not in hw_dirs


# ---------------------------------------------------------------------------
# End-to-end: scan_sources + scan_headers with chip-family filtering
# ---------------------------------------------------------------------------

class TestScanWithChipFamilyFilter:
    """Verify that scan_sources and scan_headers use chip_family parameter."""

    def test_scan_sources_accepts_chip_family(self):
        """scan_sources accepts chip_family parameter without error."""
        from embedded_cmake.scanner import scan_sources

        with tempfile.TemporaryDirectory() as tmp:
            for d in ["Hardware/APM32", "Hardware/STM32L", "Core/Src"]:
                os.makedirs(os.path.join(tmp, d), exist_ok=True)
            # Touch a dummy source file
            for f in ["Core/Src/main.c", "Hardware/STM32L/stm32l1xx_it.c",
                       "Hardware/APM32/apm32f10x_it.c"]:
                with open(os.path.join(tmp, f), "w") as fh:
                    fh.write("// dummy\n")

            result = scan_sources(tmp, chip_family="STM32L1")
            c_sources = [os.path.basename(s) for s in result.c_sources]

            assert "main.c" in c_sources
            assert "stm32l1xx_it.c" in c_sources
            assert "apm32f10x_it.c" not in c_sources, \
                "APM32 file should be filtered out"

    def test_scan_headers_accepts_chip_family(self):
        """scan_headers accepts chip_family parameter without error."""
        from embedded_cmake.scanner import scan_headers

        with tempfile.TemporaryDirectory() as tmp:
            for d in ["Hardware/APM32", "Hardware/STM32L", "Core/Inc"]:
                os.makedirs(os.path.join(tmp, d), exist_ok=True)
            # Touch header files
            for f in ["Core/Inc/main.h", "Hardware/STM32L/stm32l1xx.h",
                       "Hardware/APM32/apm32f10x.h"]:
                with open(os.path.join(tmp, f), "w") as fh:
                    fh.write("// dummy\n")

            hdr_dirs = scan_headers(tmp, chip_family="STM32L1")
            hdr_basenames = [os.path.basename(d) for d in hdr_dirs]

            assert "Inc" in hdr_basenames
            assert "STM32L" in hdr_basenames
            assert "APM32" not in hdr_basenames, \
                "APM32 header dir should be filtered out"
