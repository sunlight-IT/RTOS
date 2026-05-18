#!/usr/bin/env python3
"""ARMCC compile wrapper — converts absolute source paths to relative.

CMake always passes absolute source paths to compilers. ARMCC embeds the
path from the command line directly into DWARF debug info. VS Code
Cortex-Debug then cannot match source files for breakpoints because the
absolute paths differ between machines.

This wrapper converts ``-c <absolute>`` to ``-c <relative-from-CWD>``
before invoking armcc, so DWARF contains relative paths like
``../Core/Src/main.c`` — matching the format Keil MDK produces.

Usage (set automatically by CMake toolchain):
    python armcc-relpath-wrapper.py <armcc.exe> <flags>... -c <source> -o <obj>
"""

import os
import subprocess
import sys


def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: armcc-relpath-wrapper.py <compiler> [args...]", file=sys.stderr)
        sys.exit(1)

    compiler = sys.argv[1]
    raw_args = sys.argv[2:]

    cwd = os.getcwd()
    new_args: list[str] = []
    i = 0

    while i < len(raw_args):
        arg = raw_args[i]

        if arg == "-c" and i + 1 < len(raw_args):
            source_abs = os.path.abspath(raw_args[i + 1])
            try:
                source_rel = os.path.relpath(source_abs, cwd)
                # Use forward slashes to match Keil MDK DWARF format
                source_rel = source_rel.replace("\\", "/")
            except ValueError:
                source_rel = source_abs.replace("\\", "/")
            new_args.append(arg)
            new_args.append(source_rel)
            i += 2
        else:
            new_args.append(arg)
            i += 1

    result = subprocess.run([compiler] + new_args)
    sys.exit(result.returncode)


if __name__ == "__main__":
    main()
