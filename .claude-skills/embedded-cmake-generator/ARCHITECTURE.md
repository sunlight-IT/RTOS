# Architecture

## Pipeline Overview

```
[.uvprojx / .ioc / project directory]
          |
          v
    +------------+     +------------+     +------------+     +----------+     +-----------+     +-------------+
    |  Parsers   | --> |  Detector  | --> |   Config   | --> | Scanner  | --> | Generator | --> | CmakeWriter |
    |            |     |            |     |   Merger   |     |          |     |           |     |             |
    +------------+     +------------+     +------------+     +----------+     +-----------+     +-------------+
    uvprojx_parser     detector.py        config.py          scanner.py       generator.py       cmake_writer.py
    ioc_parser         chip_db.py                                                   |
    makefile_parser    toolchain.py                                            toolchain.py
                       utils.py                                                utils.py
```

## Data Flow

```
Input files (parsed)
    |
    v
ParsedData (dict from parsers)
    |
    v
ProjectConfig (dataclass -- the single source of truth through the pipeline)
    |
    +---> scan_sources() + scan_headers() --> ScanResult (dataclass)
    |
    +---> detect_monolithic_includes() --> MonolithicInfo (dataclass)
    |
    v
generate_all() --> CMake files (.cmake, CMakeLists.txt, wrapper scripts)
```

### Stage Details

| Stage | Input | Output | Module |
|-------|-------|--------|--------|
| Parse | .uvprojx XML / .ioc INI / Makefile | ParsedData dict | `parsers/` |
| Detect | Project directory + parsed data | ProjectConfig (partial) | `detector.py`, `chip_db.py`, `toolchain.py` |
| Merge | ProjectConfig + embedded-cmake.json | ProjectConfig (final) | `config.py` |
| Scan | Project directory + ScanConfig | ScanResult (c_sources, asm_sources, hdr_dirs) | `scanner.py`, `utils.py` |
| Generate | ProjectConfig + ScanResult + ToolchainRegistry | CMakeLists.txt, xxx-toolchain.cmake, project_config.cmake, wrapper | `generator.py`, `cmake_writer.py`, `toolchain.py` |
| Output | CMake files | Written to `cmake/` directory | `cli.py` |

## Module Dependency Graph

```
cli.py
  ├── chip_db.py ────────── json_registry.py
  ├── toolchain.py ──────── json_registry.py
  ├── config.py ─────────── detector.py
  │                           ├── chip_db.py
  │                           ├── toolchain.py
  │                           ├── parsers/uvprojx_parser.py
  │                           ├── parsers/ioc_parser.py
  │                           └── parsers/makefile_parser.py
  ├── scanner.py ────────── utils.py
  ├── generator.py ──────── cmake_writer.py, toolchain.py, utils.py
  └── utils.py
```

## Key Patterns

### JsonRegistry\<T\> (Generic Base Class)

```python
class JsonRegistry(ABC):
    """Base class for JSON data loading with builtin + user search paths."""
    def load_builtin(self) -> None: ...
    def add_search_path(self, path: str) -> None: ...
```

Used by:
- `ChipDB(JsonRegistry)` — loads chip definitions from `data/chips/*.json`
- `ToolchainRegistry(JsonRegistry)` — loads toolchain configs from `data/toolchains/*.json`

### CmakeWriter (Builder Pattern)

```python
writer = CmakeWriter()
writer.comment("Header")
writer.set("VAR", "value")
writer.list("LIST_VAR", ["a", "b", "c"])
writer.if_condition("COND")
writer.target_sources(...)
writer.endif()
```

No raw string concatenation for CMake syntax — always use CmakeWriter methods.

### Dataclass Models (Immutable-ish Data Transfer)

All pipeline state is typed dataclasses (not dicts). This enables:
- IDE autocomplete at every stage
- Type checking (mypy) for data flow errors
- Clear serialization/deserialization boundaries

### Merge-Explicit Config Override

```python
merge_configs(detected: ProjectConfig, user_data: Dict, chip_db) → None
```

User data overrides auto-detected values in-place. List fields support:
- Append: `"defines": ["MY_MACRO"]` — adds to auto-detected list
- Remove: `"exclude_dirs": ["-build"]` — `-` prefix removes from default list

## Extension Points

| What | How | Python Required? |
|------|-----|-----------------|
| New chip family | Create `data/chips/xxx.json` (use `_template.json`) | No |
| New chip model | Add entry to existing `data/chips/xxx.json` | No |
| New toolchain | Create `data/toolchains/xxx.json` + implement `compiler_method` | Minimal |
| New parser | Create `parsers/new_parser.py` implementing `find_` + `parse_` + `extract_` pattern | Yes |
| New RTOS | Add detection logic in `detector.py` + port mapping in generator | Yes |
| New CLI flag | Add to `_build_parser()` + `_apply_cli_overrides()` | Yes |
