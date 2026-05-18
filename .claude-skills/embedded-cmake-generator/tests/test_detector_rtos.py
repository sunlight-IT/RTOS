"""Tests for RTOS detection from defines and directory structure."""
from __future__ import annotations

import os

import pytest

from embedded_cmake.detector import ProjectDetector
from embedded_cmake.chip_db import ChipDB
from embedded_cmake.toolchain import ToolchainRegistry


def _create_fake_project(tmp_path, rtos_type: str):
    """Create a minimal project tree with RTOS markers."""
    (tmp_path / "Core" / "Inc").mkdir(parents=True)
    (tmp_path / "Core" / "Src").mkdir(parents=True)

    # Minimal main.c
    main_c = tmp_path / "Core" / "Src" / "main.c"
    main_c.write_text("int main(void) { while(1); }", encoding="utf-8")

    if rtos_type == "FreeRTOS":
        free_dir = tmp_path / "Middlewares" / "Third_Party" / "FreeRTOS" / "Source"
        free_dir.mkdir(parents=True)
        (free_dir / "FreeRTOSConfig.h").touch()
        (free_dir / "tasks.c").write_text("#include \"FreeRTOSConfig.h\"\n", encoding="utf-8")
        # Port directory
        port_dir = free_dir / "portable" / "GCC" / "ARM_CM3"
        port_dir.mkdir(parents=True)
        (port_dir / "port.c").touch()
        port_dir2 = free_dir / "portable" / "MemMang"
        port_dir2.mkdir(parents=True)
        (port_dir2 / "heap_4.c").touch()

    elif rtos_type == "uCOS-II":
        ucos_dir = tmp_path / "source" / "RTOS" / "uCOSII"
        ucos_dir.mkdir(parents=True)
        # Create required subdirectories
        (ucos_dir / "uC-CORE").mkdir(parents=True)
        (ucos_dir / "uC-CORE" / "ucos_ii.h").touch()
        (ucos_dir / "uC-CPU" / "ARM-Cortex-M3" / "RealView").mkdir(parents=True)
        (ucos_dir / "uC-PORT" / "ARM-Cortex-M3" / "Generic" / "RealView").mkdir(parents=True)


class TestDetectorRTOS:
    """Verify RTOS detection from project structure."""

    def _detect(self, tmp_path) -> str:
        """Run detection and return RTOS type."""
        detector = ProjectDetector(str(tmp_path))
        config = detector.detect()
        return config.rtos.type

    def test_no_rtos(self, tmp_path):
        """Project without RTOS detected as 'none'."""
        _create_fake_project(tmp_path, rtos_type="none")
        assert self._detect(tmp_path) == "none"

    def test_freertos_detected(self, tmp_path):
        """FreeRTOSConfig.h triggers FreeRTOS detection."""
        _create_fake_project(tmp_path, rtos_type="FreeRTOS")
        assert self._detect(tmp_path) == "FreeRTOS"

    def test_ucosii_detected(self, tmp_path):
        """ucos_ii.h triggers uCOS-II detection."""
        _create_fake_project(tmp_path, rtos_type="uCOS-II")
        assert self._detect(tmp_path) == "uCOS-II"

    def test_freertos_heap_selected(self, tmp_path):
        """FreeRTOS project selects heap_4.c."""
        _create_fake_project(tmp_path, rtos_type="FreeRTOS")
        detector = ProjectDetector(str(tmp_path))
        config = detector.detect()
        assert config.rtos.type == "FreeRTOS"
        assert config.rtos.heap == "heap_4.c"
