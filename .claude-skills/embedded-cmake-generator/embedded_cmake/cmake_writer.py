"""Generic CMake syntax writer.

Provides a structured builder for emitting CMake content, replacing
raw string concatenation. The writer is toolchain-agnostic — use
``if_block(condition)`` / ``else_block()`` / ``endif()`` instead of
toolchain-specific helpers.
"""

from __future__ import annotations

from typing import List


class CmakeWriter:
    """Helper to write formatted CMake output."""

    def __init__(self):
        self._lines: List[str] = []
        self._indent = 0

    def line(self, text: str = "") -> None:
        """Add a line with current indentation."""
        if text:
            self._lines.append("    " * self._indent + text)
        else:
            self._lines.append("")

    def comment(self, text: str) -> None:
        """Add a ``#`` comment line."""
        self.line(f"# {text}")

    def indent(self) -> None:
        """Increase indent level."""
        self._indent += 1

    def dedent(self) -> None:
        """Decrease indent level."""
        if self._indent > 0:
            self._indent -= 1

    def set_list(self, name: str, items: List[str]) -> None:
        """Write a CMake ``set(VAR ...)`` block with one item per line."""
        self.line(f"set({name}")
        self.indent()
        for item in items:
            self.line(item)
        self.dedent()
        self.line(")")

    def if_block(self, condition: str) -> None:
        """Open a generic ``if(...)`` block."""
        self.line(f"if({condition})")

    def else_block(self) -> None:
        self.line("else()")

    def endif(self) -> None:
        self.line("endif()")

    def if_armcc(self) -> None:
        """Shorthand for ARMCC compiler check.

        Retained for backward-compatibility during migration.
        Prefer ``if_block('CMAKE_C_COMPILER_ID MATCHES "ARMCC"')``.
        """
        self.if_block('CMAKE_C_COMPILER_ID MATCHES "ARMCC"')

    def else_armcc(self) -> None:
        """Shorthand for else() after an ARMCC check (backward compat)."""
        self.else_block()

    def list_append(self, var: str, item: str) -> None:
        """Emit ``list(APPEND VAR item)``."""
        self.line(f"list(APPEND {var} {item})")

    def list_remove(self, var: str, item: str) -> None:
        """Emit ``list(REMOVE_ITEM VAR item)``."""
        self.line(f"list(REMOVE_ITEM {var} {item})")

    def render(self) -> str:
        return "\n".join(self._lines) + "\n"
