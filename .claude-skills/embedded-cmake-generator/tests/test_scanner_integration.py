"""Integration tests for the source/header scanner.

Covers ``scan_sources``, ``scan_headers``, and
``detect_config_h_conflicts`` from ``embedded_cmake.scanner``
using temporary directory trees.
"""
from __future__ import annotations

import os
import tempfile

from embedded_cmake.models import ScanConfig
from embedded_cmake.scanner import (
    _evaluate_board_condition,
    detect_config_h_conflicts,
    scan_headers,
    scan_sources,
    suggest_board_features,
)


def _make_file(dirpath: str, rel_path: str) -> str:
    """Create a file (and parent directories) under *dirpath*."""
    full = os.path.join(dirpath, rel_path)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    # Write minimal content
    with open(full, "w") as f:
        f.write("// placeholder\n")
    return full


# ---------------------------------------------------------------------------
# scan_sources
# ---------------------------------------------------------------------------

class TestScanSources:
    """Verify source file discovery with configurable exclusions."""

    def test_finds_c_sources(self):
        """Scan finds all .c files in the project tree."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "src/main.c")
            _make_file(tmpdir, "src/gpio.c")
            _make_file(tmpdir, "README.md")
            result = scan_sources(tmpdir)
            assert len(result.c_sources) == 2
            assert any(p.endswith("main.c") for p in result.c_sources)
            assert any(p.endswith("gpio.c") for p in result.c_sources)

    def test_finds_asm_sources(self):
        """Scan finds .s / .S / .asm files."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "src/startup.s")
            _make_file(tmpdir, "src/irq.S")
            _make_file(tmpdir, "src/cpu_a.asm")
            result = scan_sources(tmpdir)
            assert len(result.asm_sources) == 3

    def test_finds_cpp_sources(self):
        """Scan finds .cpp / .cxx / .cc files."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "src/module.cpp")
            _make_file(tmpdir, "src/module.cxx")
            _make_file(tmpdir, "src/module.cc")
            result = scan_sources(tmpdir)
            assert len(result.cpp_sources) == 3

    def test_exclude_dir(self):
        """Excluded directories are skipped."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "src/main.c")
            _make_file(tmpdir, "build/out.c")
            config = ScanConfig(exclude_dirs=["build"])
            result = scan_sources(tmpdir, config)
            assert len(result.c_sources) == 1
            assert all("build" not in p for p in result.c_sources)

    def test_exclude_file(self):
        """Excluded files are skipped."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "src/main.c")
            _make_file(tmpdir, "src/syscalls.c")
            config = ScanConfig(exclude_files=["syscalls.c"])
            result = scan_sources(tmpdir, config)
            source_names = [os.path.basename(p) for p in result.c_sources]
            assert "syscalls.c" not in source_names
            assert "main.c" in source_names

    def test_exclude_file_pattern(self):
        """Excluded file patterns are skipped."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "src/main.c")
            _make_file(tmpdir, "src/template_stub.c")
            config = ScanConfig(exclude_file_patterns=["*template*"])
            result = scan_sources(tmpdir, config)
            source_names = [os.path.basename(p) for p in result.c_sources]
            assert "template_stub.c" not in source_names
            assert "main.c" in source_names

    def test_no_sources_returns_empty(self):
        """Scan with no matching files returns empty result."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "README.md")
            result = scan_sources(tmpdir)
            assert len(result.c_sources) == 0
            assert len(result.asm_sources) == 0
            assert len(result.cpp_sources) == 0

    def test_default_config_used_when_none(self):
        """Passing None for config uses ScanConfig defaults."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "src/main.c")
            result = scan_sources(tmpdir, None)
            assert len(result.c_sources) == 1

    def test_cmake_build_dir_auto_excluded(self):
        """Directories with CMakeCache.txt are auto-excluded."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "src/main.c")
            _make_file(tmpdir, "build/CMakeCache.txt")
            _make_file(tmpdir, "build/foo.c")
            result = scan_sources(tmpdir)
            source_names = [os.path.basename(p) for p in result.c_sources]
            assert "foo.c" not in source_names
            assert "main.c" in source_names


# ---------------------------------------------------------------------------
# scan_headers
# ---------------------------------------------------------------------------

class TestScanHeaders:
    """Verify header directory discovery."""

    def test_finds_header_dirs(self):
        """Scan finds directories containing .h files."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "inc/main.h")
            _make_file(tmpdir, "src/main.c")
            result = scan_headers(tmpdir)
            header_dirs = [os.path.normpath(p) for p in result]
            inc_dir = os.path.normpath(os.path.join(tmpdir, "inc"))
            assert inc_dir in header_dirs

    def test_no_headers_returns_empty(self):
        """Scan with no header files returns empty list."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "src/main.c")
            result = scan_headers(tmpdir)
            assert result == []

    def test_extra_exclude_subdirs(self):
        """Extra subdirectory names are excluded from header scan."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "Middlewares/FreeRTOS/include/list.h")
            _make_file(tmpdir, "inc/main.h")
            result = scan_headers(tmpdir, extra_exclude_subdirs=["Middlewares"])
            header_dirs = [os.path.normpath(p) for p in result]
            inc_dir = os.path.normpath(os.path.join(tmpdir, "inc"))
            middleware_dir = os.path.normpath(os.path.join(tmpdir, "Middlewares", "FreeRTOS", "include"))
            assert inc_dir in header_dirs
            assert middleware_dir not in header_dirs

    def test_exclude_dir_in_scan_config(self):
        """Exclude dirs from ScanConfig are respected."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "inc/main.h")
            _make_file(tmpdir, "build/out.h")
            config = ScanConfig(exclude_dirs=["build"])
            result = scan_headers(tmpdir, config)
            header_dirs = [os.path.normpath(p) for p in result]
            build_dir = os.path.normpath(os.path.join(tmpdir, "build"))
            assert build_dir not in header_dirs

    def test_finds_hpp_files(self):
        """Scan finds directories containing .hpp files."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "inc/header.hpp")
            result = scan_headers(tmpdir)
            assert len(result) == 1
            assert result[0].endswith("inc")


# ---------------------------------------------------------------------------
# detect_config_h_conflicts
# ---------------------------------------------------------------------------

class TestDetectConfigHConflicts:
    """Verify config.h conflict detection and priority ordering."""

    def test_single_config_h_no_reorder(self):
        """Single config.h directory returns unchanged."""
        with tempfile.TemporaryDirectory() as tmpdir:
            main_dir = os.path.join(tmpdir, "source", "Main")
            _make_file(main_dir, "config.h")
            result = detect_config_h_conflicts([main_dir], tmpdir)
            assert result == [main_dir]

    def test_no_config_h_no_reorder(self):
        """No config.h files means no reordering."""
        with tempfile.TemporaryDirectory() as tmpdir:
            d1 = os.path.join(tmpdir, "src")
            d2 = os.path.join(tmpdir, "lib")
            os.makedirs(d1)
            os.makedirs(d2)
            result = detect_config_h_conflicts([d1, d2], tmpdir)
            assert result == [d1, d2]

    def test_main_takes_priority(self):
        """Directory containing Main in its path is prioritized."""
        with tempfile.TemporaryDirectory() as tmpdir:
            main_dir = os.path.join(tmpdir, "source", "Main")
            common_dir = os.path.join(tmpdir, "source", "Common")
            _make_file(main_dir, "config.h")
            _make_file(common_dir, "config.h")
            result = detect_config_h_conflicts([common_dir, main_dir], tmpdir)
            # Main should be first (higher priority)
            assert result[0] == main_dir

    def test_app_takes_priority(self):
        """Directory containing App in its path is prioritized."""
        with tempfile.TemporaryDirectory() as tmpdir:
            app_dir = os.path.join(tmpdir, "source", "App")
            common_dir = os.path.join(tmpdir, "source", "Common")
            _make_file(app_dir, "config.h")
            _make_file(common_dir, "config.h")
            result = detect_config_h_conflicts([common_dir, app_dir], tmpdir)
            assert result[0] == app_dir

    def test_multiple_config_h_warns(self):
        """Multiple config.h directories are logged (primary first)."""
        with tempfile.TemporaryDirectory() as tmpdir:
            main_dir = os.path.join(tmpdir, "source", "Main")
            common_dir = os.path.join(tmpdir, "source", "Common")
            _make_file(main_dir, "config.h")
            _make_file(common_dir, "config.h")
            result = detect_config_h_conflicts([common_dir, main_dir], tmpdir)
            # Both present, primary first
            assert len(result) == 2
            assert main_dir in result
            assert common_dir in result


# ---------------------------------------------------------------------------
# suggest_board_features
# ---------------------------------------------------------------------------

BOARD_CONFIG_H_SAMPLE = """\
#ifndef __CONFIG_H__
#define __CONFIG_H__

#define BOARD_WLM200        21
#define BOARD_F9164A        4

#ifndef __BOARD
#define __BOARD             BOARD_F9164A
#endif

#if (__BOARD == BOARD_F9164A)
#define __FACTORYTEST       1
#define __BEEP              1
#define __LED_NET           0
#ifndef __BT
#define __BT                1
#endif
//#define __LORA              1
#elif (__BOARD == BOARD_WLM200)
#define __FACTORYTEST       1
#define __LED_BT            0
#ifndef __GPS
#define __GPS               0
#endif
#ifndef __SWPROC
#define __SWPROC            1
#endif
#else
#define __SAFEMODE          1
#endif

#if 1
#ifndef __BD
#define __BD                0
#endif
#endif

#endif
"""


class TestEvaluateBoardCondition:
    """Unit tests for ``_evaluate_board_condition``."""

    board_map = {"BOARD_WLM200": 21, "BOARD_F9164A": 4, "BOARD_LP100": 19}

    def test_matching_board(self):
        """Returns True when active board matches a condition."""
        cond = "(__BOARD == BOARD_WLM200)"
        assert _evaluate_board_condition(cond, 21, self.board_map) is True

    def test_or_chain(self):
        """Returns True when any item in an || chain matches."""
        cond = "(__BOARD == BOARD_LP100) || (__BOARD == BOARD_WLM200)"
        assert _evaluate_board_condition(cond, 21, self.board_map) is True

    def test_no_match_in_chain(self):
        """Returns False when no item matches."""
        cond = "(__BOARD == BOARD_LP100) || (__BOARD == BOARD_F9164A)"
        assert _evaluate_board_condition(cond, 21, self.board_map) is False

    def test_double_parens_ok(self):
        """Handles ((__BOARD == BOARD_XXX) || ...) wrapping."""
        cond = "((__BOARD == BOARD_LP100) || (__BOARD == BOARD_WLM200))"
        assert _evaluate_board_condition(cond, 21, self.board_map) is True

    def test_constant_one(self):
        """#if 1 returns True."""
        assert _evaluate_board_condition("1", 21, self.board_map) is True

    def test_constant_zero(self):
        """#if 0 returns False."""
        assert _evaluate_board_condition("0", 21, self.board_map) is False

    def test_non_board_condition(self):
        """Non-__BOARD conditions return False."""
        assert _evaluate_board_condition("__CPU_MK64", 21, self.board_map) is False


class TestSuggestBoardFeatures:
    """Tests for ``suggest_board_features`` with synthetic config.h."""

    def _write_config_h(self, tmpdir: str, content: str = BOARD_CONFIG_H_SAMPLE) -> str:
        dirpath = os.path.join(tmpdir, "source", "Main")
        os.makedirs(dirpath, exist_ok=True)
        path = os.path.join(dirpath, "config.h")
        with open(path, "w") as f:
            f.write(content)
        return dirpath

    def test_board_f9164a_detection(self):
        """Detects BOARD_F9164A and suggests its =1 flags."""
        import tempfile
        with tempfile.TemporaryDirectory() as tmpdir:
            hdr_dir = self._write_config_h(tmpdir)
            result = suggest_board_features(tmpdir, hdr_dir, [])
            assert result["board_name"] == "BOARD_F9164A"
            must = {f for f, _ in result["must_define"]}
            assert "__FACTORYTEST" in must
            assert "__BEEP" in must
            assert "__BT" in must

    def test_board_wlm200_detection(self):
        """Overriding __BOARD to 21 selects BOARD_WLM200 block."""
        import tempfile
        with tempfile.TemporaryDirectory() as tmpdir:
            hdr_dir = self._write_config_h(tmpdir)
            result = suggest_board_features(
                tmpdir, hdr_dir, ["__BOARD=21"],
            )
            assert result["board_name"] == "BOARD_WLM200"
            must = {f for f, _ in result["must_define"]}
            assert "__FACTORYTEST" in must
            assert "__SWPROC" in must
            # __GPS is guarded with default 0 — not suggested
            assert "__GPS" not in must

    def test_no_config_h(self):
        """Returns empty dict when no config.h exists."""
        import tempfile
        with tempfile.TemporaryDirectory() as tmpdir:
            result = suggest_board_features(tmpdir, tmpdir, [])
            assert result == {}

    def test_no_board_value(self):
        """Returns error when __BOARD cannot be determined."""
        import tempfile
        with tempfile.TemporaryDirectory() as tmpdir:
            hdr_dir = self._write_config_h(tmpdir, """
#ifndef __CONFIG_H__
#define __CONFIG_H__
#define BOARD_WLM200 21
#endif
""")
            result = suggest_board_features(tmpdir, hdr_dir, [])
            assert "error" in result

    def test_commented_out_flag(self):
        """Commented-out flags in board block appear as must_define."""
        import tempfile
        with tempfile.TemporaryDirectory() as tmpdir:
            hdr_dir = self._write_config_h(tmpdir)
            result = suggest_board_features(tmpdir, hdr_dir, ["__BOARD=4"])
            must = {f for f, _ in result["must_define"]}
            assert "__LORA" in must

    def test_never_defined_flags(self):
        """Flags referenced in #if conditions but never #defined appear."""
        import tempfile
        with tempfile.TemporaryDirectory() as tmpdir:
            hdr_dir = self._write_config_h(tmpdir, """
#ifndef __CONFIG_H__
#define __CONFIG_H__
#define BOARD_WLM200 21
#ifndef __BOARD
#define __BOARD BOARD_WLM200
#endif
#if (__BOARD == BOARD_WLM200)
#define __KNOWN 1
#endif
#if __UNKNOWN
#endif
#endif
""")
            result = suggest_board_features(tmpdir, hdr_dir, ["__BOARD=21"])
            never = {f for f, _ in result["never_defined"]}
            assert "__UNKNOWN" in never
