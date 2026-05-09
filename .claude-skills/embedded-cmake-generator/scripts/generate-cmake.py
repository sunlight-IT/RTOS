#!/usr/bin/env python3
"""STM32 CMake Project Generator (v3.0 - backward-compatible wrapper).

Delegates to the new embedded_cmake package for all functionality.
Maintains the same CLI interface as the v2.x script.

Usage:
    python generate-cmake.py [options]
    python generate-cmake.py -d /path/to/project
    python generate-cmake.py -t gcc
"""

import os
import sys

# Ensure the skill directory is on the path
_skill_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _skill_dir not in sys.path:
    sys.path.insert(0, _skill_dir)

from embedded_cmake.cli import main

if __name__ == "__main__":
    sys.exit(main())
