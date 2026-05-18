# 支持能力矩阵

## 自动检测管道

技能使用 8 遍检测（按优先级排序）：

| 遍 | 检测项 | 说明 |
|----|--------|------|
| 0 | **CubeMX 优先检查** | 若 `.ioc` 存在，跳过 Keil 检测（`.uvprojx` 为 CubeMX 导出文件） |
| 1 | **Keil MDK** | 解析 .uvprojx，提取 defines、include 路径、源文件、散列文件、库 |
| 2 | CubeMX | 解析 .ioc，提取芯片型号、RTOS、工具链 |
| 3 | 芯片头文件 | 扫描 stm32f*.h、apm32e*.h 等头文件 |
| 4 | RTOS | 检测 FreeRTOSConfig.h / ucos_ii.h |
| 5 | 工具链 | 检测 arm-none-eabi-gcc / ARMCC |
| 6 | 启动/链接文件 | 搜索 startup_*.s、*.sct、*.ld |
| 7 | 单片化 include | 检测 #include "*.c" 模式（仅 Keil 项目） |
| 8 | 项目名称 | Makefile → CMakeLists.txt → 目录名 |

## 支持的芯片

| 系列 | CPU | FPU | 状态 |
|------|-----|-----|------|
| STM32F1 | Cortex-M3 | none | 内置 |
| APM32E1 | Cortex-M3 | none | 内置 |
| STM32L1 | Cortex-M3 | none | 内置 |
| NXP MK64 | Cortex-M4 | fpv4-sp-d16 | 内置 |
| 用户自定义 | 任意 | 任意 | 使用 `_template.json` 扩展 |

> 新增芯片：在 `embedded_cmake/data/chips/` 下添加 JSON 文件，参照 `stm32f1.json` 格式。

## 支持的 RTOS

| RTOS | 检测方式 | Port 切换 |
|------|----------|-----------|
| FreeRTOS | `FreeRTOSConfig.h` | GCC ↔ RVDS (ARMCC) |
| uCOS-II | `ucos_ii.h` | GNU ↔ RealView (ARMCC) |

## Keil 项目支持

自动从 `.uvprojx` 文件提取：

- 编译器宏定义（`<Define>` 标签）
- Include 路径
- 源文件分组 + 排除标记
- 预编译库（`.lib` 文件）
- 散列加载文件
- 芯片型号 / CPU 类型
- RTOS 类型（通过宏检测）
- 单片化 include 关系
