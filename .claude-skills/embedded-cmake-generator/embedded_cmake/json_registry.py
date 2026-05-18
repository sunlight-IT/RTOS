"""Generic JSON data registry base class.

Provides shared loading logic for ``ChipDB`` and ``ToolchainRegistry``,
eliminating the duplicated ``_load_from_paths`` → ``load_path`` → ``_load_file``
pattern that existed in both classes.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Callable, Dict, Generic, List, Optional, TypeVar

T = TypeVar("T")


class JsonRegistry(Generic[T]):
    """Base class for JSON-backed data registries.

    Subclasses must implement ``_parse_item(data: dict) -> T``.

    Usage::

        class ChipDB(JsonRegistry[ChipInfo]):
            def _parse_item(self, data: dict) -> ChipInfo:
                return _parse_chip_json(data)
    """

    def __init__(self, search_paths: Optional[List[Path]] = None):
        self._items: Dict[str, T] = {}
        self._search_paths: List[Path] = list(search_paths or [])

    # -- subclass hooks ---------------------------------------------------

    def _parse_item(self, data: dict) -> T:
        """Parse a single JSON dict into the registry item type."""
        raise NotImplementedError

    def _register_item(self, item: T) -> None:
        """Register a parsed item (override for custom indexing)."""
        raise NotImplementedError

    # -- public API -------------------------------------------------------

    def add_search_path(self, path: Path) -> None:
        if path not in self._search_paths:
            self._search_paths.append(path)

    def load_builtin(self) -> int:
        """Load data from the default built-in data directory, then any
        user-supplied search paths.

        Returns the number of files loaded.
        """
        return self._load_from_paths()

    def load_path(self, path: Path) -> int:
        """Load all JSON files from a single directory."""
        count = 0
        if path.is_dir():
            for f in sorted(path.glob("*.json")):
                if self._load_file(f):
                    count += 1
        return count

    # -- internal ---------------------------------------------------------

    def _load_from_paths(self) -> int:
        count = 0
        for path in self._search_paths:
            count += self.load_path(path)
        return count

    def _load_file(self, filepath: Path) -> Optional[T]:
        try:
            with open(filepath, "r", encoding="utf-8") as f:
                data = json.load(f)
            item = self._parse_item(data)
            self._register_item(item)
            return item
        except (json.JSONDecodeError, KeyError, TypeError) as exc:
            print(f"[WARN] Failed to load {filepath}: {exc}", file=sys.stderr)
            return None

    # -- helpers ----------------------------------------------------------

    def list_ids(self) -> List[str]:
        return sorted(self._items.keys())

    def __len__(self) -> int:
        return len(self._items)

    def __contains__(self, key: str) -> bool:
        return key in self._items
