"""Chip database loader and query API.

Loads chip family definitions from JSON files and provides lookup by
family name, model name, or preprocessor define.
"""

from __future__ import annotations

import os
import re
from pathlib import Path
from typing import Dict, List, Optional

from .json_registry import JsonRegistry
from .models import ChipInfo, ChipModel, CpuInfo, LinkerFileInfo, MemoryRegion, StartFileInfo


def _default_data_dir() -> Path:
    """Get the default chip data directory relative to this module."""
    return Path(__file__).resolve().parent / "data" / "chips"


def _parse_chip_json(data: dict) -> ChipInfo:
    """Parse a chip JSON dict into a ChipInfo object."""
    cpu_data = data.get("cpu", {})
    cpu = CpuInfo(
        core=cpu_data.get("core", ""),
        fpu=cpu_data.get("fpu", "none"),
        endian=cpu_data.get("endian", "little"),
    )

    models: Dict[str, ChipModel] = {}
    for name, model_data in data.get("models", {}).items():
        memory = [
            MemoryRegion(name=m["name"], origin=m["origin"], length=m["length"])
            for m in model_data.get("memory", [])
        ]
        models[name] = ChipModel(
            name=name,
            defines=model_data.get("defines", []),
            memory=memory,
            max_clock_hz=model_data.get("max_clock_hz", 0),
        )

    startup_data = data.get("startup", {})
    startup = StartFileInfo(
        pattern=startup_data.get("pattern", ""),
        gcc_default=startup_data.get("gcc_default", ""),
        armcc_pattern=startup_data.get("armcc_pattern", ""),
        armcc_default=startup_data.get("armcc_default", ""),
        iar_pattern=startup_data.get("iar_pattern", ""),
    )

    linker_data = data.get("linker", {})
    linker = LinkerFileInfo(
        gcc_pattern=linker_data.get("gcc_pattern", ""),
        armcc_pattern=linker_data.get("armcc_pattern", ""),
        iar_pattern=linker_data.get("iar_pattern", ""),
    )

    return ChipInfo(
        family=data.get("family", ""),
        vendor=data.get("vendor", ""),
        cpu=cpu,
        models=models,
        default_defines=data.get("default_defines", []),
        header_paths=data.get("header_paths", {}),
        startup=startup,
        linker=linker,
        hal_driver_name=data.get("hal_driver_name", ""),
        ext=data.get("ext", {}),
    )


def resolve_chip_model(chip_info: ChipInfo, model_name: str) -> Optional[ChipModel]:
    """Resolve chip model with fuzzy matching.

    Handles naming variations:
    - CubeMX:  STM32F103C8T6 -> STM32F103C8Tx
    - Keil:    STM32F103C8   -> STM32F103C8Tx (bare device, no package suffix)
    """
    # Exact match
    model = chip_info.get_model(model_name)
    if model:
        return model

    # Fuzzy #1: normalize CubeMX suffix (e.g. C8T6 -> C8Tx, ZIT6 -> ZITx)
    normalized = re.sub(r'[A-Z]\d$', 'Tx', model_name)
    if normalized != model_name:
        model = chip_info.get_model(normalized)
        if model:
            return model

    # Fuzzy #2: append Tx suffix for bare Keil device names
    if not re.search(r'[A-Z]\d$', model_name[-3:]):
        for suffix in ['Tx', 'Vx', 'Rx', 'Cx', 'Zx']:
            candidate = model_name + suffix
            model = chip_info.get_model(candidate)
            if model:
                return model

    return None


class ChipDB(JsonRegistry[ChipInfo]):
    """Database of chip family definitions loaded from JSON files.

    Usage::

        db = ChipDB()
        db.add_search_path(Path("path/to/chips"))
        db.load_builtin()
        info = db.find_family("STM32F1")
        model = db.find_model("STM32F103C8Tx")
    """

    def __init__(self, search_paths: Optional[List[Path]] = None):
        super().__init__(search_paths)
        self._model_index: Dict[str, str] = {}     # model name -> family name
        self._define_index: Dict[str, str] = {}    # define -> family name

    # -- JsonRegistry hooks -----------------------------------------------

    def _parse_item(self, data: dict) -> ChipInfo:
        return _parse_chip_json(data)

    def _register_item(self, item: ChipInfo) -> None:
        self._items[item.family] = item
        for model_name in item.models:
            self._model_index[model_name] = item.family
            for define in item.models[model_name].defines:
                self._define_index[define] = item.family

    def load_builtin(self) -> int:
        """Load built-in chip definitions."""
        self.add_search_path(_default_data_dir())
        return self._load_from_paths()

    # -- lookup API -------------------------------------------------------

    def find_family(self, family: str) -> Optional[ChipInfo]:
        """Look up a chip family by name (case-insensitive)."""
        family_upper = family.upper()
        for name, info in self._items.items():
            if name.upper() == family_upper:
                return info
        return None

    def find_model(self, model: str) -> Optional[ChipModel]:
        """Look up a specific chip model by name."""
        family_name = self._model_index.get(model)
        if family_name:
            family = self._items.get(family_name)
            if family:
                return family.get_model(model)
        return None

    def find_family_for_model(self, model: str) -> Optional[ChipInfo]:
        """Look up the family containing a specific chip model.

        Supports fuzzy matching: STM32F103C8T6 matches STM32F103C8Tx in DB.
        """
        # Exact match first
        family_name = self._model_index.get(model)
        if family_name:
            return self._items.get(family_name)

        # Fuzzy match
        normalized = re.sub(r'(\d)[A-Z]\d$', r'\1Tx', model)
        if normalized != model:
            family_name = self._model_index.get(normalized)
            if family_name:
                return self._items.get(family_name)

        return None

    def find_by_define(self, define: str) -> Optional[ChipModel]:
        """Find a chip model by its preprocessor define (e.g. 'STM32F103xB')."""
        family_name = self._define_index.get(define)
        if family_name:
            family = self._items.get(family_name)
            if family:
                return family.find_model_by_define(define)
        return None

    def find_family_by_define(self, define: str) -> Optional[ChipInfo]:
        """Find a chip family by a preprocessor define."""
        family_name = self._define_index.get(define)
        if family_name:
            return self._items.get(family_name)
        return None

    def list_models(self, family: Optional[str] = None) -> List[str]:
        """List all models, optionally filtered by family."""
        if family:
            info = self.find_family(family)
            return sorted(info.models.keys()) if info else []
        return sorted(self._model_index.keys())
