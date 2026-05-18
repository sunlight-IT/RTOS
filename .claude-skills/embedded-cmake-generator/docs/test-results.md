# Test Verification Records

验证项目矩阵，用于了解 embedded-cmake-generator 的兼容性边界。每次重大测试活动追加新条目。

---

## 验证项目矩阵

| 项目 | Chip | RTOS | 格式 | 检测 | 构建 | 备注 |
|------|------|------|------|------|------|------|
| STM32F103C8T6 (Project) | STM32F1 | FreeRTOS | CubeMX+Keil | A | A | ARMCC+GCC 双工具链 |
| LORA | APM32E1 | uCOS-II | Keil | A | A | ARMCC |
| F-GROUP | MK64 | uCOS-II | Keil | A | A | ARMCC |
| G431_AIP_app | STM32G4 | FreeRTOS | CubeMX+Keil | A | A | ARMCC+GCC |
| T1 (ekuiter/stm32-projects) | STM32F103VE | uCOS-II | Keil | A | - | P1 路径 2→20 C sources |
| T2 (MaJerle/stm32f429) | STM32F407VG | bare | Keil | A | ❌ | StdPeriph `__weak` 不兼容 GCC |
| T3 (STM32CubeG0) | STM32G071RB | - | CubeMX IOC | A | ❌ | 子模块未初始化，Drivers 空 |
| T4 (STM32H7-LwIP) | STM32H743ZITx | FreeRTOS | CubeMX IOC | A | A | LwIP+ETH+PPP, .elf 328KB |
| T5 (APM32F003_blinky) | APM32F0x | bare | Makefile | C | ❌ | 缺 APM32F00x chip JSON |
| T6 (RT-Thread APM32F103) | APM32F103VB | RT-Thread | Keil | C | ❌ | 缺 GCC .ld 链接脚本 |

---

## 已知限制

1. **APM32F00x chip JSON 缺失** — Makefile-only 项目（T5）芯片未解析，回退到 cortex-m3
2. **RT-Thread BSP 缺 GCC .ld** — RT-Thread 使用 SCons 生成链接脚本，无标准 .ld 文件
3. **Makefile-only 项目无 MCU 上下文** — P5 可提取 `-mcpu`/`-D` 但无项目格式文件时定位芯片有限
4. **LwIP 子目录 include 路径** — Scanner 添加 leaf 目录（`lwip/`）而非 parent（`include/`），导致 `#include "lwip/opt.h"` 解析失败
5. **ARM_CM4F/ARM_CM7 port 重复** — FreeRTOS Scanner 发现所有 Cortex-M 变体 port 文件，未按芯片过滤

---

## 测试活动记录

### 2026-05-15：P1-P5 修复后外部项目验证

**修复内容**: P1 路径保护放宽, P2 CubeMX HAL 驱动, P3 Chip JSON 扩展, P4 RT-Thread 检测, P5 Makefile 解析增强  
**额外修复**: `chip_db.resolve_chip_model()` 模糊匹配 regex 修正  
**结果**: R1-R4 0 退化, T1/T4 检测 A 级, T4 首次外部项目 GCC 构建成功  
**详情**: 见 `_test_generality/TEST_REPORT.md` 及 `memory/p1p5_external_project_verification.md`

### 2026-05-14：v4.2.1 芯片系列过滤回归

**修复内容**: 芯片系列目录过滤, 功能宏诊断  
**结果**: STM32F103C8T6 ARMCC+GCC 0 错误, LORA 348→289 C sources  
**详情**: 见 `memory/regression_test_v4.2.1.md`
