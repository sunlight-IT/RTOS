"""Chip database loader and query API.

Loads chip family definitions from JSON files and provides lookup by
family name, model name, or preprocessor define.
"""

from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Dict, List, Optional

from .models import ChipInfo, ChipModel, CpuInfo, MemoryRegion, StartFileInfo, LinkerFileInfo


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


class ChipDB:
    """Database of chip family definitions loaded from JSON files.

    Usage::

        db = ChipDB()
        db.load_builtin()
        info = db.find_family("STM32F1")
        model = db.find_model("STM32F103C8Tx")
        model2 = db.find_by_define("STM32F103xB")
    """

    def __init__(self, search_paths: Optional[List[Path]] = None):
        self._families: Dict[str, ChipInfo] = {}  # family name -> ChipInfo
        self._model_index: Dict[str, str] = {}     # model name -> family name
        self._define_index: Dict[str, str] = {}    # define -> family name
        self._search_paths: List[Path] = list(search_paths or [])

    def add_search_path(self, path: Path) -> None:
        """Add a search path for JSON chip files."""
        if path not in self._search_paths:
            self._search_paths.append(path)

    def load_builtin(self) -> int:
        """Load all built-in chip definitions. Returns count of files loaded."""
        self._search_paths.append(_default_data_dir())
        return self._load_from_paths()

    def load_path(self, path: Path) -> int:
        """Load chip JSON files from a specific directory. Returns count loaded."""
        count = 0
        if path.is_dir():
            for f in sorted(path.glob("*.json")):
                self._load_file(f)
                count += 1
        return count

    def _load_from_paths(self) -> int:
        """Load from all registered search paths."""
        count = 0
        for path in self._search_paths:
            count += self.load_path(path)
        return count

    def _load_file(self, filepath: Path) -> Optional[ChipInfo]:
        """Load a single chip JSON file."""
        try:
            with open(filepath, "r", encoding="utf-8") as f:
                data = json.load(f)
            chip = _parse_chip_json(data)
            self.register(chip)
            return chip
        except (json.JSONDecodeError, KeyError, TypeError) as e:
            import sys
            print(f"[WARN] Failed to load chip file {filepath}: {e}", file=sys.stderr)
            return None

    def register(self, chip: ChipInfo) -> None:
        """Register a ChipInfo (from JSON or programmatic creation)."""
        self._families[chip.family] = chip
        for model_name in chip.models:
            self._model_index[model_name] = chip.family
            for define in chip.models[model_name].defines:
                self._define_index[define] = chip.family

    def find_family(self, family: str) -> Optional[ChipInfo]:
        """Look up a chip family by name (case-insensitive)."""
        family_upper = family.upper()
        for name, info in self._families.items():
            if name.upper() == family_upper:
                return info
        return None

    def find_model(self, model: str) -> Optional[ChipModel]:
        """Look up a specific chip model by name."""
        family_name = self._model_index.get(model)
        if family_name:
            family = self._families.get(family_name)
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
            return self._families.get(family_name)

        # Fuzzy match: normalize number suffixes (e.g. C8T6 -> C8Tx)
        return self._fuzzy_match_model(model)

    def _fuzzy_match_model(self, model: str) -> Optional[ChipInfo]:
        """Try fuzzy matching for chip model names.

        Handles CubeMX naming differences like:
        - STM32F103C8T6 vs STM32F103C8Tx (pin count + package suffix)
        """
        import re
        # Normalize: strip trailing package info after the density code
        # e.g. STM32F103C8T6 -> base STM32F103C8
        # Actually, match by dropping last chars and replacing with wildcard
        normalized = re.sub(r'(\d)[A-Z]\d$', r'\1Tx', model)
        if normalized != model:
            family_name = self._model_index.get(normalized)
            if family_name:
                return self._families.get(family_name)
        return None

    def find_by_define(self, define: str) -> Optional[ChipModel]:
        """Find a chip model by its preprocessor define (e.g. 'STM32F103xB')."""
        family_name = self._define_index.get(define)
        if family_name:
            family = self._families.get(family_name)
            if family:
                return family.find_model_by_define(define)
        return None

    def find_family_by_define(self, define: str) -> Optional[ChipInfo]:
        """Find a chip family by a preprocessor define."""
        family_name = self._define_index.get(define)
        if family_name:
            return self._families.get(family_name)
        return None

    def list_families(self) -> List[str]:
        """List all registered family names."""
        return sorted(self._families.keys())

    def list_models(self, family: Optional[str] = None) -> List[str]:
        """List all models, optionally filtered by family."""
        if family:
            info = self.find_family(family)
            return sorted(info.models.keys()) if info else []
        return sorted(self._model_index.keys())

    def __len__(self) -> int:
        return len(self._families)

    def __contains__(self, family: str) -> bool:
        return self.find_family(family) is not None
