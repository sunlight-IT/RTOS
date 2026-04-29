---
name: cmsis-dap-debug
description: STM32F103C8T6 CMSIS-DAP 调试技能，使用 OpenOCD 进行内存、寄存器和执行控制
version: 1.0.0
tags: [embedded, stm32, debug, openocd, cmsis-dap, gdb]
---

# CMSIS-DAP Debug Skill

STM32F103C8T6 调试技能，使用 OpenOCD 和 CMSIS-DAP 调试器进行嵌入式调试。支持内存操作、寄存器访问、断点管理和执行控制。

## When to Use

使用此 skill 当你需要：

- 调试 STM32F103C8T6 固件
- 读取和修改内存数据
- 查看和修改寄存器值
- 设置断点进行逐步调试
- 分析崩溃和故障
- 验证固件运行状态

## 快速开始

### 基本使用

```bash
# 读取 FLASH 区域前 16 字节
python .claude-skills/cmsis-dap-debug/scripts/debug.py --read-memory 0x08000000 --count 4

# 读取 RAM 区域前 16 字节
python .claude-skills/cmsis-dap-debug/scripts/debug.py --read-memory 0x20000000 --count 4

# 显示所有寄存器
python .claude-skills/cmsis-dap-debug/scripts/debug.py --show-registers

# 暂停目标
python .claude-skills/cmsis-dap-debug/scripts/debug.py --halt

# 单步执行
python .claude-skills/cmsis-dap-debug/scripts/debug.py --step

# 设置断点
python .claude-skills/cmsis-dap-debug/scripts/debug.py --set-breakpoint 0x08000100

# 复位目标
python .claude-skills/cmsis-dap-debug/scripts/debug.py --reset --halt
```

### 外设寄存器调试

```bash
# 显示 GPIOA 寄存器
python .claude-skills/cmsis-dap-debug/scripts/debug.py --show-peripheral GPIOA

# 显示 USART1 寄存器
python .claude-skills/cmsis-dap-debug/scripts/debug.py --show-peripheral USART1
```

---

## 功能特性

### 内存操作

| 功能 | 命令 | 说明 |
|------|--------|------|
| 读取内存 | `--read-memory <addr>` | 读取指定地址的内存 |
| 指定数量 | `--count <num>` | 读取数量（默认: 1）|
| 数据大小 | `--size {w,h,b}` | w=32位, h=16位, b=8位 |

### 寄存器操作

| 功能 | 命令 | 说明 |
|------|--------|------|
| 显示寄存器 | `--show-registers [name]` | 显示所有或指定寄存器 |
| 外设寄存器 | `--show-peripheral <name>` | 显示外设寄存器 (GPIOA, GPIOB, GPIOC, USART1, USART2) |

### 断点管理

| 功能 | 命令 | 说明 |
|------|--------|------|
| 设置断点 | `--set-breakpoint <addr>` | 在指定地址设置断点 |
| 硬件断点 | `--hardware-bp` | 使用硬件断点 |
| 移除断点 | `--remove-breakpoint <num>` | 移除断点编号 |
| 列出断点 | `--list-breakpoints` | 显示所有断点 |

### 执行控制

| 功能 | 命令 | 说明 |
|------|--------|------|
| 暂停 | `--halt` | 暂停目标执行 |
| 恢复 | `--resume` | 恢复目标执行 |
| 单步 | `--step` | 单步执行 |
| 复位 | `--reset` | 复位目标 |
| 复位并暂停 | `--reset --hardware-bp` | 复位后保持暂停 |

---

## 内存区域

STM32F103C8T6 内存映射：

| 区域 | 起始地址 | 结束地址 | 大小 | 访问 |
|------|---------|---------|------|------|
| FLASH | 0x08000000 | 0x08010000 | 64KB | RX (读/执行) |
| RAM | 0x20000000 | 0x20005000 | 20KB | RW (读/写) |
| 外设 | 0x40000000 | 0x60000000 | 512MB | RW |
| 系统 | 0xE0000000 | 0xE0100000 | 1MB | RW |

---

## 外设寄存器

### GPIO

| 外设 | 基地址 | 寄存器 |
|------|--------|--------|
| GPIOA | 0x40010800 | CRL, CRH, IDR, ODR, BSRR, BRR, LCKR |
| GPIOB | 0x40010C00 | CRL, CRH, IDR, ODR, BSRR, BRR, LCKR |
| GPIOC | 0x40011000 | CRL, CRH, IDR, ODR, BSRR, BRR, LCKR |

### USART

| 外设 | 基地址 | 寄存器 |
|------|--------|--------|
| USART1 | 0x40013800 | SR, DR, BRR, CR1, CR2, CR3, GTPR |
| USART2 | 0x40004400 | SR, DR, BRR, CR1, CR2, CR3, GTPR |

---

## 核心寄存器

Cortex-M3 核心寄存器：

| 组 | 寄存器 | 说明 |
|----|--------|------|
| 通用 | r0-r12 | 通用目的寄存器 |
| 特殊 | sp (r13) | 栈指针 |
| | lr (r14) | 链接寄存器 |
| | pc (r15) | 程序计数器 |
| 状态 | xpsr | 组合程序状态寄存器 |
| | apsr | 应用程序状态寄存器 |
| | ipsr | 中断程序状态寄存器 |
| 控制 | primask | 优先级屏蔽寄存器 |
| | basepri | 基本优先级屏蔽寄存器 |
| | faultmask | 故障屏蔽寄存器 |
| | control | 控制寄存器 |

---

## OpenOCD 命令参考

### 内存命令

```bash
# 读取 32 位字
mdw <addr> [count]

# 读取 16 位半字
mdh <addr> [count]

# 读取 8 位字节
mdb <addr> [count]

# 写入 32 位字
mww <addr> <value>

# 写入 16 位半字
mwh <addr> <value>

# 写入 8 位字节
mwb <addr> <value>
```

### 寄存器命令

```bash
# 显示所有寄存器
reg

# 显示指定寄存器
reg <name>

# 写入寄存器
reg <name> <value>
```

### 断点命令

```bash
# 设置断点
bp <addr> [hw]

# 移除断点
rbp <num>

# 列出断点
bp
```

### 执行控制命令

```bash
# 暂停目标
halt [ms]

# 恢复执行
resume [addr]

# 单步执行
step [addr]

# 复位目标
reset [halt]

# 等待暂停
wait_halt [ms]
```

---

## 常见调试流程

### 1. 启动调试

```bash
# 初始化、复位、暂停
python .claude-skills/cmsis-dap-debug/scripts/debug.py --reset --hardware-bp

# 查看寄存器
python .claude-skills/cmsis-dap-debug/scripts/debug.py --show-registers
```

### 2. 设置断点并运行

```bash
# 设置断点
python .claude-skills/cmsis-dap-debug/scripts/debug.py --set-breakpoint 0x08000100

# 恢复执行
python .claude-skills/cmsis-dap-debug/scripts/debug.py --resume
```

### 3. 单步调试

```bash
# 暂停目标
python .claude-skills/cmsis-dap-debug/scripts/debug.py --halt

# 单步执行
python .claude-skills/cmsis-dap-debug/scripts/debug.py --step

# 查看寄存器
python .claude-skills/cmsis-dap-debug/scripts/debug.py --show-registers
```

### 4. 崩溃分析

```bash
# 暂停目标
python .claude-skills/cmsis-dap-debug/scripts/debug.py --halt

# 查看寄存器
python .claude-skills/cmsis-dap-debug/scripts/debug.py --show-registers

# 查看栈
python .claude-skills/cmsis-dap-debug/scripts/debug.py --read-memory 0x20000000 --count 16
```

---

## 配置选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| --interface | cmsis-dap | 调试器接口 |
| --target | stm32f1x.cfg | 目标配置文件 |
| --transport | swd | 传输协议 |
| --speed | 1000 | 调试速度 kHz |
| --output | debug_output.log | 输出文件 |

---

## 故障排查

### Q: OpenOCD 找不到调试器？

**症状：** `Error: couldn't find cmsis-dap`

**解决：**
1. 检查 USB 连接
2. 确认 CMSIS-DAP 调试器已正确连接
3. Windows 下可能需要以管理员身份运行

### Q: 目标无法暂停？

**症状：** `Error: target not halted`

**解决：**
1. 确认目标芯片正在运行
2. 检查 BOOT0/BOOT1 引脚配置
3. 尝试复位后暂停：`--reset --hardware-bp`

### Q: 无法设置断点？

**症状：** `Error: can't set breakpoint`

**原因：** Cortex-M3 最多支持 6 个硬件断点

**解决：**
1. 列出当前断点：`--list-breakpoints`
2. 移除不需要的断点：`--remove-breakpoint <num>`
3. 使用硬件断点：`--hardware-bp`

### Q: 内存读取失败？

**症状：** `Error: memory access failed`

**解决：**
1. 确认地址在有效内存区域内
2. FLASH 区域只能读取，不能写入
3. 检查外设时钟是否已使能

### Q: 寄存器值不正确？

**症状：** 寄存器显示的值不符合预期

**解决：**
1. 确认目标已暂停：`--halt`
2. 重新读取寄存器：`--show-registers`
3. 检查是否在中断处理程序中

---

## 输出文件

所有调试输出会自动保存到 `debug_output.log` 文件，包含：

- 时间戳
- 调试器配置
- OpenOCD 标准输出
- OpenOCD 标准错误

文件格式：
```
============================================================
时间: 2026-04-17 15:30:00
调试器: cmsis-dap
目标: stm32f1x.cfg
传输: swd
速度: 1000 kHz
============================================================

[STDOUT]
<OpenOCD 输出内容>

[STDERR]
<OpenOCD 错误输出>
```

---

## 与其他技能集成

### embedded-flasher

烧录后立即开始调试：

```bash
# 先烧录
bash .claude-skills/embedded-flasher/scripts/flash.sh

# 然后调试
python .claude-skills/cmsis-dap-debug/scripts/debug.py --reset --hardware-bp
```

### embedded-cmake-generator

完整开发流程：

```bash
# 1. 生成 CMake 配置
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py

# 2. 构建
mkdir -p build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-toolchain.cmake
make -j4

# 3. 烧录
cd ..
bash .claude-skills/embedded-flasher/scripts/flash.sh

# 4. 调试
python .claude-skills/cmsis-dap-debug/scripts/debug.py --reset --hardware-bp
```

---

## 高级功能

### 直接使用 OpenOCD

如果需要更复杂的调试功能，可以直接使用 OpenOCD：

```bash
openocd \
    -c "interface cmsis-dap" \
    -c "transport select swd" \
    -f target/stm32f1x.cfg \
    -c "adapter speed 1000" \
    -c "halt" \
    -c "reg" \
    -c "mdw 0x08000000 4" \
    -c "shutdown"
```

### 使用 GDB

连接 GDB 到 OpenOCD：

```bash
# 启动 OpenOCD 服务器
openocd -f interface/cmsis-dap.cfg -f target/stm32f1x.cfg

# 在另一个终端启动 GDB
arm-none-eabi-gdb build/Project.elf
(gdb) target remote localhost:3333
(gdb) monitor halt
(gdb) b main
(gdb) continue
```

---

## 相关 Skills

- **embedded-flasher** - STM32 固件烧录工具
- **embedded-cmake-generator** - STM32 项目 CMake 工程生成器
- **embedded-compiler** - STM32 ARM GCC 编译指导
