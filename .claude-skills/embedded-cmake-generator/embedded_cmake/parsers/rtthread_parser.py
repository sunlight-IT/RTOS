"""RT-Thread ``rtconfig.h`` parser.

RT-Thread BSP projects always contain an ``rtconfig.h`` at the project root,
which defines all RTOS feature flags as ``#define RT_USING_XXX`` macros.

The parser extracts enabled macros as build defines so they are available
for CMake generation.
"""

from __future__ import annotations

import os
import re
from typing import List


def parse_rtconfig(filepath: str) -> List[str]:
    """Parse an ``rtconfig.h`` file and extract enabled preprocessor defines.

    Extracts:
    - ``#define RT_USING_XXX`` (feature enables)
    - ``#define RT_XXX`` (core configuration)
    - ``#define BSP_USING_XXX`` (board-specific features)
    - ``#define PKG_USING_XXX`` (package enables)

    Macros explicitly set to ``0`` are skipped (disabled).

    Args:
        filepath: Path to ``rtconfig.h``.

    Returns:
        List of define strings (values appended as ``NAME=VALUE`` when present).
    """
    defines: List[str] = []
    if not os.path.isfile(filepath):
        return defines

    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    pattern = re.compile(r"#define\s+(\w+)\s*(.*)")

    for line in content.splitlines():
        line = line.strip()
        # Skip comments
        if line.startswith("//") or line.startswith("/*"):
            continue
        m = pattern.match(line)
        if m:
            name = m.group(1)
            value = m.group(2).strip()
            if value == "0":
                # Explicitly disabled — skip
                continue
            if not value:
                # Simple define (no value)
                defines.append(name)
            else:
                # Remove trailing C comments
                value = value.split("//")[0].split("/*")[0].strip()
                defines.append(f"{name}={value}")

    return defines
