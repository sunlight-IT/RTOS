"""End-to-end tests for the embedded CMake generator.

Covers the full pipeline from source scanning to CMake generation,
with focus on the scanner's ability to discover pre-compiled library
files (.lib, .a) and the generator's ability to link them.
"""
from __future__ import annotations

import os
import shutil
import tempfile

from embedded_cmake.config import load_config
from embedded_cmake.generator import generate_all
from embedded_cmake.models import ScanConfig
from embedded_cmake.scanner import scan_headers, scan_sources


def _make_file(dirpath: str, rel_path: str) -> str:
    """Create a file (and parent directories) under *dirpath*."""
    full = os.path.join(dirpath, rel_path)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    with open(full, "w") as f:
        f.write("// placeholder\n")
    return full


# ---------------------------------------------------------------------------
# Scanner tests for .lib / .a discovery
# ---------------------------------------------------------------------------

class TestScannerFindsLib:
    """Scanner discovers pre-compiled library files in the project tree."""

    def test_scanner_finds_dot_lib(self):
        """.lib files are discovered and placed in lib_files."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "src/main.c")
            _make_file(tmpdir, "lib/driver.lib")
            result = scan_sources(tmpdir)
            assert len(result.lib_files) == 1
            assert result.lib_files[0].endswith("driver.lib")

    def test_scanner_finds_dot_a(self):
        """.a files are discovered and placed in static_lib_files."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "src/main.c")
            _make_file(tmpdir, "lib/libdriver.a")
            result = scan_sources(tmpdir)
            assert len(result.static_lib_files) == 1
            assert result.static_lib_files[0].endswith("libdriver.a")

    def test_scanner_finds_both_lib_and_a(self):
        """Both .lib and .a files are discovered in the same project."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "src/main.c")
            _make_file(tmpdir, "lib/driver.lib")
            _make_file(tmpdir, "lib/libdriver.a")
            result = scan_sources(tmpdir)
            assert len(result.lib_files) == 1
            assert len(result.static_lib_files) == 1

    def test_lib_not_in_c_sources(self):
        """.lib files are NOT added to c_sources."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "src/main.c")
            _make_file(tmpdir, "lib/driver.lib")
            result = scan_sources(tmpdir)
            assert len(result.c_sources) == 1
            assert all(not p.endswith(".lib") for p in result.c_sources)

    def test_scanner_respects_excluded_dirs_for_lib(self):
        """Excluded directories are not scanned for .lib files."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "src/main.c")
            _make_file(tmpdir, "build/driver.lib")
            config = ScanConfig(exclude_dirs=["build"])
            result = scan_sources(tmpdir, config)
            assert len(result.lib_files) == 0

    def test_scanner_no_lib_files_returns_empty(self):
        """No .lib files in the project returns empty lib_files."""
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "src/main.c")
            result = scan_sources(tmpdir)
            assert len(result.lib_files) == 0
            assert len(result.static_lib_files) == 0


# ---------------------------------------------------------------------------
# Full pipeline tests (load_config → scan → generate_all)
# ---------------------------------------------------------------------------

class TestFullPipelineWithLib:
    """End-to-end pipeline verification with .lib files."""

    def test_delete_and_regenerate(self, chip_db, tc_registry):
        """Delete generated CMake files, regenerate — output must be identical
        in structure and still contain .lib references.
        """
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "src/main.c")
            _make_file(tmpdir, "src/main.h")
            _make_file(tmpdir, "lib/driver.lib")

            # -- First generation --
            config = load_config(tmpdir, chip_db, tc_registry)
            scan = scan_sources(tmpdir, config.scan)
            hdr_dirs = scan_headers(tmpdir, config.scan)

            # Scanner must discover the .lib file
            assert len(scan.lib_files) >= 1
            assert any("driver.lib" in f for f in scan.lib_files)

            generated = generate_all(
                tmpdir, config.cmake_dir, config, scan, hdr_dirs,
                toolchain_registry=tc_registry,
            )
            cmakelists = os.path.join(tmpdir, "CMakeLists.txt")
            assert os.path.isfile(cmakelists)

            with open(cmakelists) as f:
                first_content = f.read()
            assert "driver.lib" in first_content

            # -- Delete generated files as if starting fresh --
            os.remove(cmakelists)
            shutil.rmtree(os.path.join(tmpdir, config.cmake_dir), ignore_errors=True)

            # -- Regenerate --
            config2 = load_config(tmpdir, chip_db, tc_registry)
            scan2 = scan_sources(tmpdir, config2.scan)
            hdr_dirs2 = scan_headers(tmpdir, config2.scan)

            generated2 = generate_all(
                tmpdir, config2.cmake_dir, config2, scan2, hdr_dirs2,
                toolchain_registry=tc_registry,
            )

            cmakelists2 = os.path.join(tmpdir, "CMakeLists.txt")
            assert os.path.isfile(cmakelists2)

            with open(cmakelists2) as f:
                second_content = f.read()
            assert "driver.lib" in second_content
            assert "target_link_options" in second_content

    def test_full_pipeline_no_cmakelists(self, chip_db, tc_registry):
        """First-time generation (no pre-existing generated files)
        with .lib files in the project tree.

        This simulates the exact scenario that failed in G431 validation:
        a clean checkout with no CMakeLists.txt, where the scanner must
        discover .lib files that were previously only found via uvprojx.
        """
        with tempfile.TemporaryDirectory() as tmpdir:
            _make_file(tmpdir, "src/main.c")
            _make_file(tmpdir, "src/main.h")
            _make_file(tmpdir, "lib/level_sensing_sub.lib")

            # No generated files exist yet
            assert not os.path.isfile(os.path.join(tmpdir, "CMakeLists.txt"))

            # Full pipeline: detect → scan → generate
            config = load_config(tmpdir, chip_db, tc_registry)
            scan = scan_sources(tmpdir, config.scan)
            hdr_dirs = scan_headers(tmpdir, config.scan)

            # Scanner found the .lib via filesystem walk (not uvprojx)
            assert len(scan.lib_files) >= 1
            assert any("level_sensing_sub.lib" in f for f in scan.lib_files)

            generated = generate_all(
                tmpdir, config.cmake_dir, config, scan, hdr_dirs,
                toolchain_registry=tc_registry,
            )

            # CMakeLists.txt must reference the .lib file
            cmakelists = os.path.join(tmpdir, "CMakeLists.txt")
            assert os.path.isfile(cmakelists)

            with open(cmakelists) as f:
                content = f.read()
            assert "level_sensing_sub.lib" in content
            assert "target_link_options" in content
