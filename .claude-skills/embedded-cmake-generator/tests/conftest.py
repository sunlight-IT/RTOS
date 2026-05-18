"""Pytest fixtures for embedded CMake generator tests."""
from __future__ import annotations

from pathlib import Path
from typing import Dict

import pytest

from embedded_cmake.chip_db import ChipDB
from embedded_cmake.toolchain import ToolchainRegistry
from embedded_cmake.models import ProjectConfig, CpuInfo, ChipInfo


def _data_dir() -> Path:
    """Get the embedded_cmake data directory."""
    return Path(__file__).resolve().parent.parent / "embedded_cmake" / "data"


@pytest.fixture(scope="session")
def chip_db() -> ChipDB:
    """Session-scoped chip database loaded with built-in data."""
    db = ChipDB()
    db.add_search_path(_data_dir() / "chips")
    db.load_builtin()
    return db


@pytest.fixture(scope="session")
def tc_registry() -> ToolchainRegistry:
    """Session-scoped toolchain registry loaded with built-in data."""
    reg = ToolchainRegistry()
    reg.add_search_path(_data_dir() / "toolchains")
    reg.load_builtin()
    return reg


@pytest.fixture
def minimal_config() -> ProjectConfig:
    """A minimal ProjectConfig for generator tests."""
    return ProjectConfig(
        project_name="TestProject",
        project_dir="/tmp/test_project",
        chip_family="STM32F1",
        chip_model="STM32F103C8Tx",
        chip_info=ChipInfo(
            family="STM32F1",
            vendor="STMicroelectronics",
            cpu=CpuInfo(core="Cortex-M3", fpu="none"),
        ),
    )


@pytest.fixture
def stm32f1_chip(chip_db: ChipDB) -> ChipInfo:
    """Concrete STM32F1 ChipInfo fixture."""
    info = chip_db.find_family("STM32F1")
    assert info is not None, "STM32F1 chip data not loaded"
    return info


@pytest.fixture
def gcc_tc(tc_registry: ToolchainRegistry):
    """Concrete GCC ToolchainConfig fixture."""
    tc = tc_registry.get("gcc")
    assert tc is not None, "GCC toolchain not loaded"
    return tc


@pytest.fixture
def armcc_tc(tc_registry: ToolchainRegistry):
    """Concrete ARMCC ToolchainConfig fixture."""
    tc = tc_registry.get("armcc")
    assert tc is not None, "ARMCC toolchain not loaded"
    return tc
