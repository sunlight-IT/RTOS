# Contributing to embedded-cmake-generator

## Before You Start

Read [ARCHITECTURE.md](ARCHITECTURE.md) — understand the module structure and data flow.

## Quick Start

```bash
# Clone and enter the skill directory
cd .claude-skills/embedded-cmake-generator/

# Create virtual environment
python -m venv .venv
source .venv/bin/activate  # Windows: .venv\Scripts\activate

# Install in dev mode with test dependencies
pip install -e ".[dev]"

# Verify setup
ruff check .
pytest
```

## Development Workflow

1. **Understand the architecture** — Read ARCHITECTURE.md to find where your change fits

2. **Write tests first (TDD)** — Create or update tests in `tests/` before implementing

3. **Implement with data-driven approach**
   - Does your change add data (chip, toolchain, pattern)? → Add JSON file in `data/`
   - Does your change add behavior? → Implement in the appropriate module
   - No new hardcoded values in Python code

4. **Run full test suite**
   ```bash
   pytest --cov=embedded_cmake --cov-report=term-missing
   ```
   Coverage target: ≥ 80%

5. **Run lint**
   ```bash
   ruff check .
   ruff format --check .
   ```

6. **Update docs**
   - New feature → `docs/capabilities.md`
   - Bug fix → `docs/troubleshooting.md`
   - New chip/RTOS/toolchain → `docs/capabilities.md`
   - CHANGELOG.md entry always required

7. **Cross-project verification** (for functional changes)
   - STM32F103C8T6 + FreeRTOS (ARMCC + GCC)
   - LORA STM32L151RE + uCOS-II (ARMCC + GCC)
   - F-GROUP APM32E103RE + uCOS-II (ARMCC)

## Code Style

| Rule | Standard |
|------|----------|
| Formatter | `ruff format` (line-length = 100) |
| Linter | `ruff check` with project rules |
| Imports | Relative imports within package, absolute outside |
| Type hints | Required on all public function signatures |
| Line length | Max 100 characters |

## Testing Guidelines

- Use pytest fixtures from `tests/conftest.py` (session-scoped: `chip_db`, `tc_registry`, `minimal_config`, `stm32f1_chip`, `gcc_tc`, `armcc_tc`)
- Name test files `test_<module>.py`
- Name test functions `test_<feature>`
- Coverage target: ≥ 80% line coverage for modified modules
- Mark slow or integration tests with `@pytest.mark.slow`

### Cross-Project Verification

For changes that affect build output, verify against all reference projects:

```bash
# Each project has its own embedded-cmake.json and build procedure
# Verify ARMCC build (requires Windows + Keil MDK)
cd <project_dir>/build
cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/armcc-toolchain.cmake
make -j4  # Expect 0 errors

# Verify GCC build
cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-toolchain.cmake
make -j4  # Expect 0 errors
```

Reference projects:
| Project | Chip | RTOS | Toolchains |
|---------|------|------|------------|
| STM32F103C8T6 | STM32F103C8T6 | FreeRTOS | ARMCC, GCC |
| LORA | STM32L151RE | uCOS-II | ARMCC, GCC |
| F-GROUP | APM32E103RE | uCOS-II | ARMCC |

## Security Guidelines

- **XXE prevention**: XML parsers must use `etree.XMLParser(resolve_entities=False)` — do not use raw `etree.parse()`
- **Path traversal**: Validate that extracted file paths do not escape the project root via `../`
- **Shell injection**: Never pass user-controlled paths directly to shell commands without quoting
- **Secrets**: Never hardcode API keys, tokens, or credentials

## PR Checklist

Before submitting a pull request:

- [ ] Tests pass: `pytest`
- [ ] Lint passes: `ruff check .`
- [ ] Coverage ≥ 80%: `pytest --cov=embedded_cmake`
- [ ] Docs updated (troubleshooting.md / capabilities.md)
- [ ] CHANGELOG.md entry added
- [ ] Cross-project build verified (if functional change)
- [ ] No new hardcoded values in Python (data goes in JSON)
- [ ] XML parsers use `resolve_entities=False`

## Release Process

1. Bump version: edit `embedded_cmake/__init__.py` (`__version__`)
2. Update CHANGELOG.md with release date
3. Cross-project verification (all reference projects)
4. Tag release: `git tag v<version>` && `git push --tags`
