"""Tests for the uvprojx parser (Keil project files)."""
from __future__ import annotations

import os
import xml.etree.ElementTree as ET
from pathlib import Path

import pytest

from embedded_cmake.parsers import uvprojx_parser


def _sample_uvprojx() -> str:
    """Return a minimal valid .uvprojx XML string for testing."""
    return """<?xml version="1.0" encoding="UTF-8"?>
<Project>
  <ProjectOptions>
    <Targets>
      <Target>
        <TargetName>Target 1</TargetName>
        <TargetOption>
          <TargetArmAds>
            <Cads>
              <VariousControls>
                <Define>USE_HAL_DRIVER, STM32F103xB</Define>
                <IncludePath>Core\\Inc;Drivers\\CMSIS\\Include</IncludePath>
                <MiscControls></MiscControls>
              </VariousControls>
              <uC99>1</uC99>
              <uGnu>0</uGnu>
              <Optim>2</Optim>
            </Cads>
            <Aads>
              <VariousControls>
                <Define>__MICROLIB</Define>
              </VariousControls>
            </Aads>
            <LDads>
              <ScatterFile>MDK-ARM/Project/Project.sct</ScatterFile>
              <Misc>--library_type=microlib</Misc>
            </LDads>
          </TargetArmAds>
        </TargetOption>
        <Groups>
          <Group>
            <GroupName>Core</GroupName>
            <Files>
              <File>
                <FileName>main.c</FileName>
                <FileType>1</FileType>
                <FilePath>Core\\Src\\main.c</FilePath>
              </File>
              <File>
                <FileName>gpio.c</FileName>
                <FileType>1</FileType>
                <FilePath>Core\\Src\\gpio.c</FilePath>
              </File>
            </Files>
          </Group>
        </Groups>
      </Target>
    </Targets>
  </ProjectOptions>
</Project>"""


class TestUvprojxParser:
    """Verify uvprojx file parsing."""

    def _write_and_parse(self, tmp_path: Path, xml: str, subdirs=None):
        """Helper: create uvprojx file + optional dirs, run parser."""
        uvprojx = tmp_path / "test.uvprojx"
        uvprojx.write_text(xml, encoding="utf-8")

        # Create source file stubs referenced in the XML
        src = tmp_path / "Core" / "Src"
        src.mkdir(parents=True, exist_ok=True)
        (src / "main.c").touch()
        (src / "gpio.c").touch()

        if subdirs:
            for d in subdirs:
                (tmp_path / d).mkdir(parents=True, exist_ok=True)

        return uvprojx_parser.parse_uvprojx(str(uvprojx))

    def test_parse_defines(self, tmp_path: Path):
        """Defines are correctly extracted from the <Define> element."""
        data = self._write_and_parse(tmp_path, _sample_uvprojx())
        defines = uvprojx_parser.extract_defines(data)
        assert "USE_HAL_DRIVER" in defines
        assert "STM32F103xB" in defines

    def test_parse_include_paths(self, tmp_path: Path):
        """Include paths are extracted and normalized."""
        data = self._write_and_parse(tmp_path, _sample_uvprojx(),
                                     subdirs=["Core/Inc", "Drivers/CMSIS/Include"])
        paths = uvprojx_parser.extract_include_paths(data)
        assert any("Core" in p.replace("\\", "/") for p in paths)

    def test_parse_scatter_file(self, tmp_path: Path):
        """Scatter file path is extracted."""
        data = self._write_and_parse(tmp_path, _sample_uvprojx())
        scatter = uvprojx_parser.extract_scatter_file(data)
        assert scatter is not None
        assert "Project.sct" in scatter

    def test_parse_gnu_mode_disabled(self, tmp_path: Path):
        """uGnu=0 correctly sets gnu_mode=False."""
        data = self._write_and_parse(tmp_path, _sample_uvprojx())
        assert data.get("gnu_mode") is False

    def test_parse_gnu_mode_enabled(self, tmp_path: Path):
        """uGnu=1 correctly sets gnu_mode=True."""
        xml = _sample_uvprojx().replace("<uGnu>0</uGnu>", "<uGnu>1</uGnu>")
        data = self._write_and_parse(tmp_path, xml)
        assert data.get("gnu_mode") is True

    def test_parse_optimization(self, tmp_path: Path):
        """Optimization level is extracted as -O flag."""
        data = self._write_and_parse(tmp_path, _sample_uvprojx())
        assert data.get("optimization") == "-O2"

    def test_parse_groups(self, tmp_path: Path):
        """Source groups are extracted."""
        data = self._write_and_parse(tmp_path, _sample_uvprojx())
        groups = data.get("groups", [])
        assert len(groups) == 1
        assert groups[0].group_name == "Core"
        assert len(groups[0].source_files) == 2
