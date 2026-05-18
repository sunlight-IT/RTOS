---
name: embedded-cmake-generator
description: 通用嵌入式项目CMake工程自动生成器 (Keil/CubeMX解析, 多芯片, 多RTOS, 多工具链)
version: 4.2.1
tags: [embedded, stm32, apm32, nxp, cmake, keil, build-system, cmake-generator, armcc, gcc, ucos, freertos, universal]
---

# Embedded CMake Generator Skill

通用嵌入式 CMake 构建系统生成器。自动检测 Keil MDK (.uvprojx) 或 CubeMX (.ioc) 项目，生成 ARMCC/GCC 双工具链 CMake 构建系统。配置驱动，零配置可用。

## When to Use

- 为嵌入式项目生成 CMake 构建系统（STM32、APM32、GD32、NXP 等）
- 从 Keil MDK 项目迁移到 CMake（自动解析 .uvprojx）
- 从 CubeMX 项目生成 CMake 构建
- 自动检测项目配置（芯片型号、RTOS、工具链）
- 生成 Keil 兼容的相对路径 DWARF 信息（`../Core/Src/main.c`），确保 VS Code Cortex-Debug 断点和源码导航正常工作

## Hard Rules

1. **Keil projects**: Parse .uvprojx as the authoritative source of defines, includes, sources, and libs. Never scan the filesystem for sources when a Keil project exists.
2. **ARMCC builds**: Always use the `armcc-relpath-wrapper.py` to convert absolute paths to relative (`../Core/Src/main.c`) so DWARF debug info matches VS Code Cortex-Debug.
3. **No silent toolchain switch**: If ARMCC is not found, report the missing path and stop. Never silently fall back to GCC for a project that targets ARMCC.
4. **Config priority**: Keil .uvprojx > CubeMX .ioc > auto-detection heuristics.
5. **Output dir**: Generated CMake files go to `cmake/` by default; never write to `build_keil/` or any directory that may contain user IDE project files.

## 快速调用

```bash
# 零配置 — 自动检测项目类型
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py

# 指定工具链
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py --toolchain armcc
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py --toolchain gcc

# 指定 Keil 项目 / 芯片 / 预览
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py --keil-project path/to/project.uvprojx
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py --cpu APM32E103RE
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py --dry-run
```

或直接请求 Claude Code：`请为当前项目生成 CMake 工程配置`

## 构建

```bash
mkdir -p build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/armcc-toolchain.cmake
make -j4
```

GCC 将 `armcc-toolchain.cmake` 替换为 `arm-none-eabi-toolchain.cmake`。

## 自动检测管道

检测优先级（`.ioc` 存在时跳过 Keil 路径）：

| 遍 | 检测项 | 说明 |
|----|--------|------|
| 1 | Keil MDK | 解析 .uvprojx → defines, includes, sources, scatter, libs |
| 2 | CubeMX | 解析 .ioc → chip model, RTOS, toolchain |
| 3 | 芯片头文件 | 扫描 stm32f*.h, apm32e*.h 等 |
| 4 | RTOS | FreeRTOSConfig.h / ucos_ii.h |
| 5 | 工具链 | arm-none-eabi-gcc / ARMCC |
| 6 | 启动/链接文件 | startup_*.s, *.sct, *.ld |
| 7 | 单片化 include | `#include "*.c"` 模式（仅 Keil 项目） |
| 8 | 项目名称 | Makefile → CMakeLists.txt → 目录名 |

## 生成的文件

| 文件 | 用途 |
|------|------|
| `CMakeLists.txt` | 主构建文件（ARMCC/GCC 双工具链） |
| `cmake/project_config.cmake` | 源文件列表、include 目录、链接脚本 |
| `cmake/armcc-toolchain.cmake` | ARMCC 工具链配置（含相对路径包装器） |
| `cmake/arm-none-eabi-toolchain.cmake` | GCC 工具链配置 |
| `cmake/armcc-relpath-wrapper.py` | ARMCC 编译包装器（绝对路径→相对路径） |

## 治理体系

所有贡献者必须在提交前阅读：

| 文档 | 用途 |
|------|------|
| [CONTRIBUTING.md](CONTRIBUTING.md) | 贡献者工作流、代码风格、测试、安全 |
| [ARCHITECTURE.md](ARCHITECTURE.md) | 模块依赖图、数据流、扩展点 |

CI 阻断合并的 2 个硬性检查：
1. `ruff check .` — 零错误
2. `pytest --cov=embedded_cmake --cov-fail-under=80` — 全部通过

## 内部模块 (v4.2.0)

| 模块 | 职责 |
|------|------|
| `cli.py` | CLI 入口编排 |
| `config.py` | 配置加载/合并 |
| `detector.py` | 项目自动检测（芯片/RTOS/工具链） |
| `scanner.py` | 源文件和头文件扫描 |
| `generator.py` | CMake 内容生成 |
| `cmake_writer.py` | CMake 语法构建器 |
| `chip_db.py` | 芯片数据库查询 |
| `toolchain.py` | 工具链配置与标志解析 |
| `json_registry.py` | JSON 数据加载基类 |
| `models.py` | 数据模型定义 |
| `utils.py` | 共享工具函数（日志/路径/IO/单片化检测） |
| `parsers/` | Keil .uvprojx / CubeMX .ioc 解析器 |

## 关键参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `-d, --project-dir` | 项目根目录 | 当前目录 |
| `-o, --output-dir` | CMake 文件输出目录 | cmake/ |
| `-t, --toolchain` | 工具链 (armcc/gcc) | armcc |
| `--keil-project` | 指定 .uvprojx 文件 | 自动检测 |
| `--cpu` | 芯片型号覆盖 | 自动检测 |
| `--board` | 板级名称 | 自动检测 |
| `--dry-run` | 预览检测结果 | - |
| `--init` | 生成配置模板 | - |
| `uses_microlib` | Keil .uvprojx → useUlib | `true` | 裸机 ARMCC 是否使用 Microlib |

## 参考文档

| 文档 | 何时查阅 |
|------|----------|
| [README.md](README.md) | 完整使用指南、安装依赖 |
| [CHANGELOG.md](CHANGELOG.md) | 版本更新历史 |
| [docs/capabilities.md](docs/capabilities.md) | 支持的芯片、RTOS、Keil 提取能力 |
| [docs/toolchain-reference.md](docs/toolchain-reference.md) | ARMCC vs GCC 对比、配置细节、验证清单 |
| [docs/troubleshooting.md](docs/troubleshooting.md) | 常见问题与解决方案 |
| [docs/scan-rules.md](docs/scan-rules.md) | 源文件扫描规则、自定义排除 |
| [embedded-cmake.json.example](embedded-cmake.json.example) | 配置文件模板 |

## 相关 Skills

- [embedded-flasher](../embedded-flasher/README.md) — STM32 固件烧录
- [cmsis-dap-debug](../cmsis-dap-debug/README.md) — CMSIS-DAP 调试

