# CMSIS-DAP Debug - STM32 调试工具

使用 OpenOCD 和 CMSIS-DAP 调试器调试 STM32F103C8T6 固件。支持内存操作、寄存器访问、断点管理和执行控制。

## 功能特性

- **内存操作** - 读取和写入任意内存地址
- **寄存器访问** - 查看和修改核心寄存器及外设寄存器
- **断点管理** - 设置、移除、列出断点（最多 6 个硬件断点）
- **执行控制** - halt、resume、step、reset
- **外设支持** - GPIO、USART 等常用外设寄存器查看
- **数据持久化** - 自动保存调试输出到日志文件

## 快速开始

### 安装依赖

确保已安装 OpenOCD：

```bash
# Windows: 下载 openocd.exe
# Linux:
sudo apt install openocd
```

### 基本使用

```bash
# 读取 FLASH 区域前 16 字节
python .claude-skills/cmsis-dap-debug/scripts/debug.py --read-memory 0x08000000 --count 4

# 显示所有寄存器
python .claude-skills/cmsis-dap-debug/scripts/debug.py --show-registers

# 暂停目标
python .claude-skills/cmsis-dap-debug/scripts/debug.py --halt

# 单步执行
python .claude-skills/cmsis-dap-debug/scripts/debug.py --step
```

## 使用示例

### 内存操作

```bash
# 读取 FLASH（32 位）
python debug.py --read-memory 0x08000000 --count 4 --size w

# 读取 RAM（16 位）
python debug.py --read-memory 0x20000000 --count 8 --size h

# 读取外设寄存器（8 位）
python debug.py --read-memory 0x40010800 --count 4 --size b
```

### 寄存器操作

```bash
# 显示所有核心寄存器
python debug.py --show-registers

# 显示 PC 寄存器
python debug.py --show-registers pc

# 显示 GPIOA 外设寄存器
python debug.py --show-peripheral GPIOA
```

### 断点管理

```bash
# 设置断点
python debug.py --set-breakpoint 0x08000100

# 设置硬件断点
python debug.py --set-breakpoint 0x08000100 --hardware-bp

# 列出所有断点
python debug.py --list-breakpoints

# 移除断点
python debug.py --remove-breakpoint 0
```

### 执行控制

```bash
# 暂停目标
python debug.py --halt

# 恢复执行
python debug.py --resume

# 单步执行
python debug.py --step

# 复位目标
python debug.py --reset

# 复位并暂停
python debug.py --reset --hardware-bp
```

## 调试流程

### 标准调试流程

```bash
# 1. 复位并暂停
python debug.py --reset --hardware-bp

# 2. 查看初始状态
python debug.py --show-registers

# 3. 设置断点
python debug.py --set-breakpoint 0x08000100

# 4. 恢复执行
python debug.py --resume

# 5. 断点触发后单步调试
python debug.py --step
python debug.py --show-registers
```

### 崩溃分析流程

```bash
# 1. 暂停目标
python debug.py --halt

# 2. 查看寄存器（特别是 PC, LR, SP）
python debug.py --show-registers

# 3. 查看栈内容
python debug.py --read-memory 0x20000000 --count 16

# 4. 查看故障寄存器
python debug.py --read-memory 0xE000ED2C --count 1
```

## 支持的外设

| 外设 | 命令 | 说明 |
|------|--------|------|
| GPIOA | `--show-peripheral GPIOA` | GPIO Port A |
| GPIOB | `--show-peripheral GPIOB` | GPIO Port B |
| GPIOC | `--show-peripheral GPIOC` | GPIO Port C |
| USART1 | `--show-peripheral USART1` | USART1 |
| USART2 | `--show-peripheral USART2` | USART2 |

## 配置选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `--interface` | cmsis-dap | 调试器接口 |
| `--target` | stm32f1x.cfg | 目标配置文件 |
| `--transport` | swd | 传输协议 |
| `--speed` | 1000 | 调试速度 (kHz) |
| `--output` | debug_output.log | 输出文件 |

## 内存区域

STM32F103C8T6 内存映射：

| 区域 | 地址范围 | 大小 | 访问 |
|------|---------|------|------|
| FLASH | 0x08000000 - 0x08010000 | 64KB | RX |
| RAM | 0x20000000 - 0x20005000 | 20KB | RW |
| 外设 | 0x40000000 - 0x60000000 | 512MB | RW |

## 输出文件

调试输出自动保存到 `debug_output.log`：

```
============================================================
时间: 2026-04-17 15:30:00
调试器: cmsis-dap
目标: stm32f1x.cfg
传输: swd
速度: 1000 kHz
============================================================

[STDOUT]
<OpenOCD 输出>
```

## 常见问题

### OpenOCD 找不到调试器？

检查 USB 连接和驱动，Windows 下可能需要管理员权限。

### 目标无法暂停？

使用 `--reset --hardware-bp` 复位后暂停。

### 断点设置失败？

Cortex-M3 最多支持 6 个硬件断点，使用 `--list-breakpoints` 查看。

### 内存访问失败？

确认地址在有效内存区域内，FLASH 只读不可写。

## 相关技能

- [embedded-flasher](../embedded-flasher/) - 固件烧录
- [embedded-cmake-generator](../embedded-cmake-generator/) - CMake 生成
