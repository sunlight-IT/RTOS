"""Tests for chip database lookups."""
from __future__ import annotations

from embedded_cmake.chip_db import resolve_chip_model


class TestChipDB:
    """Verify chip database loads and resolves correctly."""

    def test_stm32f1_family(self, chip_db):
        """STM32F1 family is loaded with correct metadata."""
        info = chip_db.find_family("STM32F1")
        assert info is not None
        assert info.family == "STM32F1"
        assert info.vendor == "STMicroelectronics"
        assert info.cpu.core == "Cortex-M3"

    def test_stm32f1_defines(self, chip_db):
        """STM32F1 has expected default defines."""
        info = chip_db.find_family("STM32F1")
        assert info is not None
        assert "USE_HAL_DRIVER" in info.default_defines

    def test_stm32f103_model(self, chip_db):
        """STM32F103C8Tx model resolves with correct defines."""
        info = chip_db.find_family("STM32F1")
        assert info is not None
        model = resolve_chip_model(info, "STM32F103C8Tx")
        assert model is not None
        assert "STM32F103xB" in model.defines
        assert model.max_clock_hz == 72_000_000

    def test_stm32f103c8_memory(self, chip_db):
        """STM32F103C8 has 64KB FLASH and 20KB RAM."""
        info = chip_db.find_family("STM32F1")
        assert info is not None
        model = resolve_chip_model(info, "STM32F103C8Tx")
        assert model is not None
        flash = model.get_memory("FLASH")
        assert flash is not None
        ram = model.get_memory("RAM")
        assert ram is not None

    def test_apm32e1_family(self, chip_db):
        """APM32E1xx family is loaded."""
        info = chip_db.find_family("APM32E1")
        assert info is not None
        assert info.vendor == "Geehy"

    def test_unknown_family(self, chip_db):
        """Unknown family returns None."""
        info = chip_db.find_family("NONEXISTENT")
        assert info is None

    def test_find_by_model(self, chip_db):
        """find_family_for_model resolves STM32F103C8Tx."""
        info = chip_db.find_family_for_model("STM32F103C8Tx")
        assert info is not None
        assert info.family == "STM32F1"
