# Changelog

## v4.2.2 (2026-05-15)

- **P1 路径保护放宽**：`uvprojx_parser.py` `_is_path_within_project()` 改为 `_is_path_within_scope(max_up=3)`，允许 sibling 目录引用（max_up=3），解决 Keil 项目 `../../Libraries/` 路径被拦截问题。影响 T1/T2/T6
- **P2 CubeMX HAL 驱动路径检测**：`ioc_parser.py` 新增 `find_cubemx_repo_root()`（max_depth=8），`detector.py` 新增 `_detect_cubemx_drivers()`，自动扫描 `Drivers/<hal>/Src/*.c` 并过滤 `*template*` 文件
- **P3 Chip JSON 扩展**：新增 `gd32f3xx.json`、`tm4c.json`、`apm32f10x.json`
- **P4 RT-Thread 检测**：新增 `rtthread_parser.py`，通过 `rtconfig.h` 解析 RT-Thread 特性宏；`rtos_config.json` 添加 rtthread 条目
- **P5 Makefile 解析增强**：`makefile_parser.py` 新增 `extract_defines()`、`extract_cpu_flags()`；`detector.py` 新增 `_detect_from_makefile()`
- **修复 chip_db.resolve_chip_model() 模糊匹配**：regex `(\d)[A-Z]\d$` → `[A-Z]\d$`，支持 ZIT6→ZITx 等多字母 package code 后缀（如 STM32H743ZIT6）
- 交叉验证通过：STM32F103C8T6 GCC/ARMCC / LORA ARMCC / F-GROUP ARMCC / G431_AIP_app GCC/ARMCC (R1-R4 回归，0 退化) + T4 STM32H7-LwIP GCC (外部项目首次构建成功，.elf 328KB)

## v4.2.1 (2026-05-13)

- **新增 STM32G4 系列芯片支持**：`stm32g4xx.json` — Cortex-M4 FPv4-SP-D16，包含 STM32G431K6Ux/K8Ux/G473/G474 型号
- **修复预编译 .lib 库文件在 CubeMX+Keil 混合项目中未检测**：`detector.py` 移除 `if not ioc_file` 守卫，Keil 检测无论 .ioc 是否存在始终运行。解决了 G431 项目中 `level_sensing_sub.lib` 被静默跳过的问题
- **修复 CubeMX .ioc 芯片型号括号污染**：`ioc_parser.py` 新增 `Mcu.UserName` 回退，解决 `Mcu.Name` 格式 `STM32G431K(6-8-B)Ux` 括号剥离后变为 `STM32G431KUx`（缺少 "6"）无法匹配芯片数据库的问题
- **修复 ARMCC 5 FPU 标志不兼容**：`armcc.json` `fpv4-sp-d16` 从 `--fpu=FPv4_SP` 改为 `--fpu=vfpv4`，解决 ARMCC 5.6.960 拒绝编译的问题
- **新增 Source Insight 备份文件扫描排除**：`scan_defaults.json` 添加 `*si4project*` 排除模式，防止 `Sourceprj/xxx(1234).c` 等备份文件污染源文件扫描
- **G4 芯片 JSON `default_defines` 置空**：STM32G4 系列可同时使用 HAL 或 LL 驱动，`default_defines` 设为 `[]`，由 uvprojx 实际配置驱动
- 交叉验证通过：STM32G431K6U6 ARMCC LL 驱动 (.lib 预编译库)，0 错误
- **修复 ARMCC `--scatter` 标志泄漏到 `armasm.exe` 导致链接失败**：`generator.py` 在生成工具链文件中新增 `CMAKE_ASM_LINK_EXECUTABLE` 强制使用 `armlink.exe` 进行 ASM 链接，防止 CMake 在目标仅有 ASM 源时默认使用 `armasm.exe` 作为链接器（armasm 拒绝 `--scatter` 等链接器专用标志）
- **修复 Keil 源组空列表覆盖文件扫描结果**：`cli.py` 仅在 Keil `.uvprojx` 实际解析到源文件时才覆盖文件系统扫描结果，防止路径解析失败时 C_SOURCES 被清空
- **修复 `.lib` 文件因项目根目录推断错误被全部拒绝**：`uvprojx_parser.py` `_infer_project_dir()` 依赖 `CMakeLists.txt` 或 `source/` 目录推断项目根，首次生成（无 CMakeLists.txt）或深层嵌套 uvprojx 场景下回退到 uvprojx 自身目录，导致 `_is_path_within_project()` 拒绝所有文件（`Skipping path outside project tree`）。`parse_uvprojx()` 改为接受调用方显式传入的 `project_dir`，`detector.py` 和 `cli.py` 传递已知正确的项目根
- **修复 `__pycache__` 导致代码更新后生成结果不变**：`scripts/generate-cmake.py` 启动时自动清除 `__pycache__` 并设 `sys.dont_write_bytecode` 禁写新缓存

## v4.2.0 (2026-05-13)

- **新增自动重构准则系统**：当技能目录的 Git 提交数达到阈值时，pre-commit 钩子提示运行审计。审计引擎 `audit_principles.py` 覆盖全部 10 条设计准则（P1-P10），生成合规评分报告
- **新增 `scripts/refactor_state.py`**：基于 Git 的变更计数器，管理 `_refactor_state.json` 状态文件，提供 `--check` 入口供 pre-commit 调用
- **新增 `scripts/audit_principles.py`**：10 准则审计引擎，支持 `--fix`（自动修复 P8 版本不一致）、`--json`（机器可读输出）
- 审计系统自身遵循 P1-P10 准则：纯 stdlib、数据驱动、路径安全、零运行时依赖

- **修复 scan 排除列表无法移除默认项**：`config.py` 的 `_merge_scan_config()` 新增 `-` 前缀移除语法。用户在 `embedded-cmake.json` 中配置 `"exclude_dirs": ["-build"]` 可从默认排除列表移除 `build/` 目录
- **修复 uvprojx .lib 预编译库文件路径解析失败**：`uvprojx_parser.py` 的 `_extract_source_groups()` 新增项目根目录回退。当相对 `.uvprojx` 目录解析失败时，尝试相对项目根目录解析，解决深层嵌套 uvprojx 中相对路径 lib 文件被静默跳过的问题
- **修复 `_header_patterns.json` 位置错误导致 ChipDB 初始化崩溃**：将 `_header_patterns.json` 从 `data/chips/` 移至 `data/` 根目录，避免被 `JsonRegistry` 误当作芯片数据解析
- 交叉验证通过：STM32F103+FreeRTOS (ARMCC/GCC) / LORA STM32L151RE+uCOS-II (ARMCC/GCC) / F-GROUP APM32E103RE+uCOS-II (ARMCC)，全部 5 项 0 错误

## v4.1.2 (2026-05-14)

- **修复 uvprojx 编译器选项 `uGnu`/`uC99`/`Optim` 未检测**：`_extract_compiler_settings` 从 `<VariousControls>`（错误）而非 `<Cads>`（正确）查找这些元素，导致 ARMCC `--gnu` 标志从未生效，使用 GNU 扩展语法的 Keil 项目（如 LORA 项目）编译失败（12 个 `#159` 函数声明不兼容错误）
- 新增 `--gnu` 条件生成：`generator.py` 将硬编码的 `--c99` 改为从 config 读取，仅在 uvprojx `<uGnu>1</uGnu>` 时追加 `--gnu` 标志
- 新增 `ProjectConfig.armcc_gnu_mode` 字段，打通从 uvprojx → config → generator 的完整数据流
- 交叉验证通过：LORA (STM32L151+uCOS-II, `uGnu=1`) / STM32F103+FreeRTOS (无 `uGnu`) ，ARMCC 0 错误

## v4.1.1 (2026-05-13)

- **修复 ARMCC Microlib 缺失导致设备无法启动**：裸机 ARMCC 项目必须使用 `--library_type=microlib` 链接标志，否则 `__main` 会走标准 C 库初始化路径（`__rt_entry` → `_platform_post_stackheap_init` → `__rt_lib_init`），这些板级支持函数在裸机上不存在，导致设备上电后无法跳转到 `main()`
- 新增 `__MICROLIB` 汇编预定义：通过 `--pd "__MICROLIB SETA 1"` 使启动文件 (.s) 走 Microlib 代码路径（`EXPORT __initial_sp`），而非标准库路径（`IMPORT __use_two_region_memory`）
- 新增 `uses_microlib` 配置字段：`ProjectConfig.uses_microlib` 默认为 `True`（裸机安全默认），从 Keil `.uvprojx` 的 `useUlib` 元素自动检测
- 修复 Keil `IncludeInBuild=0` 解析遗漏：`uvprojx_parser.py` 中 XPath 从 `fe.find("IncludeInBuild")` 改为 `fe.find(".//IncludeInBuild")`，支持嵌套在 `FileOption/CommonProperty` 下的排除标志（如 `qspi.c` 排除构建）
- 交叉验证通过：STM32F103+FreeRTOS / APM32E103+uCOS-II / APM32E103 bare-metal，ARMCC 0 错误

## v4.1.0 (2026-05-13)

- **全面重构优化**：代码行数从 ~3616 减至 ~2800 (↓22%)，消除 3 处重复代码
- 新增 `utils.py` — 统一日志、路径标准化、文件 I/O、单片化检测（消除 3 处重复）
- 新增 `json_registry.py` — JSON 数据加载基类（ChipDB + ToolchainRegistry 复用）
- 新增 `cmake_writer.py` — 通用化 CmakeWriter（移除非泛用 ARMCC 专用方法）
- 提取 `armcc-relpath-wrapper.py` 为独立文件（移除生成器中的字符串字面量）
- 精简 `detector.py` — 742 行 → 450 行 (↓39%)，使用共享单片化检测和芯片模型解析
- 精简 `generator.py` — 727 行 → 370 行 (↓49%)，移除硬编码路径
- CPU 架构映射移至 JSON 数据文件（data/toolchains/*.json），支持数据驱动
- 移除冗余 `scripts/generate-cmake.sh`（310 行 bash 重复实现）
- 移除 `-DNO_USE_ZTHREAD_CHECK` 硬编码定义
- 交叉验证通过：STM32F103+FreeRTOS + APM32E103+uCOS-II，ARMCC 0 错误

## v4.0.3 (2026-05-12)

- **修复 CMake ARMCC 构建的 DWARF 路径问题**：源文件路径从绝对路径（`D:\WorkSpace\...\Core\Src\main.c`）改为相对路径（`../Core/Src/main.c`），与 Keil MDK 生成的格式一致，便于 VS Code Cortex-Debug 断点和源码导航
- 修复 `-g` 调试标志仅存在于 `DEBUG_INIT` 的问题，移至 `CMAKE_C_FLAGS_INIT` 确保始终生成完整 DWARF 信息
- 新增 `armcc-relpath-wrapper.py` 编译包装器，在 armcc 调用前自动将绝对源文件路径转换为相对路径
- 覆盖 `CMAKE_C_COMPILE_OBJECT` / `CMAKE_CXX_COMPILE_OBJECT` 规则以注入包装器

## v4.0.2 (2026-05-12)

- 修复扫描器自动排除 CMake 构建目录（检测 `CMakeCache.txt`，不再仅依赖 `build*` 名称匹配）
- 修复非标准名称构建目录（如 `_b`）中的 CMake 测试文件被编译到目标的问题
- **诊断 Cortex-Debug + OpenOCD 调试问题**：ARMCC ELF 的 `RW_IRAM1` PROGBITS 段不在 PT_LOAD 段中，GDB `load` 无法正确初始化 `.data` 段。解决方案：Cortex-Debug 使用 `loadFiles` 指定 `.hex` 文件，通过 OpenOCD `program` 命令加载。详见 [docs/troubleshooting.md](docs/troubleshooting.md) 问题 8

## v4.0.1 (2026-05-09)

- 修复 CubeMX/Keil 检测优先级（`.ioc` 存在时跳过 Keil 路径）
- 修复芯片型号模糊匹配（Keil bare name `STM32F103C8` → `STM32F103C8Tx`）
- 修复 ARMCC ASM 启动文件隔离（GCC 启动文件不再泄漏到 ARMCC 分支）
- 修复 Monolithic 检测范围（仅 Keil 项目运行）
- 交叉项目测试通过：STM32F103+FreeRTOS / APM32E103+uCOS-II

## v4.0.0 (2026-05-09)

- Keil MDK .uvprojx 解析器 — 自动提取编译器宏、include 路径、源文件分组、散列文件、链接库
- 多 RTOS 支持 — FreeRTOS + uCOS-II（自动检测 port 路径、ASM 文件）
- 芯片数据库扩展 — 新增 APM32E1、STM32L1、NXP MK64 + 用户自定义模板
- 单片化 include 检测 — 自动识别 `#include "*.c"` 模式，仅编译根文件
- config.h 冲突检测 — 项目存在多个 config.h 时自动调整优先级
- 预编译库支持 — 自动链接 Keil .lib 库文件
- 更健壮的散列文件搜索 — 多路径搜索（KeilPrj/、Hardware/、MDK-ARM/）

## v3.0.0 (2026-04-17)

- 配置驱动架构 — `embedded-cmake.json` 配置文件支持
- 芯片数据库 — JSON 格式芯片定义，按需扩展
- 工具链数据库 — CPU/FPU 标志自动解析
- 多芯片/多项目自动检测
- 零配置使用 + 全参数可定制

## v2.0.0 (2026-04-17)

- 添加 ARMCC/GCC 双工具链支持
- 修复所有已知问题

## v1.1.0 (2026-04-17)

- 从 GCC 迁移到 ARMCC
