"""Tests for toolchain flag resolution from JSON."""
from __future__ import annotations

from embedded_cmake.models import CpuInfo


class TestToolchainFlags:
    """CPU/FPU flags resolve correctly from JSON data."""

    def test_gcc_cortex_m3(self, gcc_tc):
        """GCC Cortex-M3 with no FPU returns -mcpu=cortex-m3 -mthumb -mfloat-abi=soft."""
        cpu = CpuInfo(core="Cortex-M3", fpu="none")
        flags = gcc_tc.resolve_cpu_flags(cpu)
        assert "-mcpu=cortex-m3" in flags
        assert "-mthumb" in flags
        assert "-mfloat-abi=soft" in flags

    def test_gcc_cortex_m4_fpu(self, gcc_tc):
        """GCC Cortex-M4 with fpv4-sp-d16 returns hardware FPU flags."""
        cpu = CpuInfo(core="Cortex-M4", fpu="fpv4-sp-d16")
        flags = gcc_tc.resolve_cpu_flags(cpu)
        assert "-mcpu=cortex-m4" in flags
        assert "-mfloat-abi=hard" in flags
        assert "-mfpu=fpv4-sp-d16" in flags

    def test_armcc_cortex_m3(self, armcc_tc):
        """ARMCC Cortex-M3 returns --cpu=Cortex-M3."""
        cpu = CpuInfo(core="Cortex-M3", fpu="none")
        flags = armcc_tc.resolve_cpu_flags(cpu)
        assert flags == "--cpu=Cortex-M3"

    def test_armcc_cortex_m4_fpu(self, armcc_tc):
        """ARMCC Cortex-M4 with fpv4-sp-d16 returns FPU flags."""
        cpu = CpuInfo(core="Cortex-M4", fpu="fpv4-sp-d16")
        flags = armcc_tc.resolve_cpu_flags(cpu)
        assert "--cpu=Cortex-M4" in flags
        assert "--fpu=vfpv4" in flags

    def test_gcc_unknown_core(self, gcc_tc):
        """GCC with unknown core returns only FPU flag (no CPU flag)."""
        cpu = CpuInfo(core="Unknown-Core", fpu="none")
        flags = gcc_tc.resolve_cpu_flags(cpu)
        # cpu_flag resolves to "" (unknown core), fpu_flag resolves to "-mfloat-abi=soft" (none)
        assert flags == "-mfloat-abi=soft"

    def test_toolchain_cxx_flags(self, armcc_tc):
        """ARMCC CXX flags contain expected --cpp --no_exceptions."""
        cxx = armcc_tc.flags.get("cxx_common", [])
        assert "--cpp" in cxx
        assert "--no_exceptions" in cxx

    def test_toolchain_executable_suffix(self, armcc_tc, gcc_tc):
        """Toolchain executable suffixes match expectations."""
        assert armcc_tc.executable_suffix == ".axf"
        assert gcc_tc.executable_suffix == ".elf"
