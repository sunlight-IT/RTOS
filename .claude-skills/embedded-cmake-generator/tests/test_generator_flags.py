"""Tests for generator flag output — data-driven generation proofs."""
from __future__ import annotations

import os

from embedded_cmake.cmake_writer import CmakeWriter
from embedded_cmake.generator import (
    generate_armcc_toolchain,
    generate_gcc_toolchain,
)
from embedded_cmake.models import ProjectConfig, ChipInfo, CpuInfo


class TestGeneratorFlags:
    """Verify generated CMake files contain expected data-driven flags.

    These tests validate that the generator reads from config (which in
    production receives values from the toolchain JSON / uvprojx parser)
    rather than producing hardcoded output.
    """

    def test_armcc_optim_from_config(self, minimal_config: ProjectConfig):
        """ARMCC optimization flag comes from config, not hardcoded -Otime."""
        minimal_config.armcc_optimization = "-O2"
        w = CmakeWriter()
        generate_armcc_toolchain(w, minimal_config)
        content = w.render()
        assert "-O2" in content
        assert "-Otime" not in content

    def test_armcc_optim_default(self, minimal_config: ProjectConfig):
        """ARMCC defaults to -Otime when no optimization configured."""
        minimal_config.armcc_optimization = ""
        w = CmakeWriter()
        generate_armcc_toolchain(w, minimal_config)
        content = w.render()
        assert "-Otime" in content

    def test_armcc_gnu_flag(self, minimal_config: ProjectConfig):
        """ARMCC --gnu flag appears when armcc_gnu_mode is True."""
        minimal_config.armcc_gnu_mode = True
        w = CmakeWriter()
        generate_armcc_toolchain(w, minimal_config)
        content = w.render()
        assert "--gnu" in content

    def test_armcc_no_gnu_flag(self, minimal_config: ProjectConfig):
        """ARMCC --gnu flag is absent when armcc_gnu_mode is False."""
        minimal_config.armcc_gnu_mode = False
        w = CmakeWriter()
        generate_armcc_toolchain(w, minimal_config)
        content = w.render()
        assert "--gnu" not in content

    def test_armcc_misc_cflags(self, minimal_config: ProjectConfig):
        """ARMCC misc compiler flags appear in generated output."""
        minimal_config.armcc_misc_cflags = "--diag_suppress=66"
        w = CmakeWriter()
        generate_armcc_toolchain(w, minimal_config)
        content = w.render()
        assert "--diag_suppress=66" in content

    def test_armcc_misc_ldflags(self, minimal_config: ProjectConfig):
        """ARMCC misc linker flags appear in linker flags init."""
        minimal_config.armcc_misc_ldflags = "--diag_suppress=6439"
        w = CmakeWriter()
        generate_armcc_toolchain(w, minimal_config)
        content = w.render()
        assert "--diag_suppress=6439" in content

    def test_armcc_asm_defines(self, minimal_config: ProjectConfig):
        """ARMCC asm defines generate SETA 1 directives."""
        minimal_config.armcc_asm_defines = ["__MICROLIB", "MY_FEATURE"]
        w = CmakeWriter()
        generate_armcc_toolchain(w, minimal_config)
        content = w.render()
        assert '--pd \\"__MICROLIB SETA 1\\"' in content
        assert '--pd \\"MY_FEATURE SETA 1\\"' in content

    def test_gcc_cpu_flags(self, minimal_config: ProjectConfig):
        """GCC generates correct CPU flags from config chip info."""
        minimal_config.chip_info = ChipInfo(
            family="STM32F4",
            vendor="STMicroelectronics",
            cpu=CpuInfo(core="Cortex-M4", fpu="fpv4-sp-d16"),
        )
        w = CmakeWriter()
        generate_gcc_toolchain(w, minimal_config)
        content = w.render()
        assert "-mcpu=cortex-m4" in content
        assert "-mthumb" in content
