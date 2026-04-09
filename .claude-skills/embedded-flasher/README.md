# Embedded Flasher - STM32 固件烧录工具

自动烧录 STM32 固件，配合 **embedded-cmake-generator** 实现完整的构建烧录流程。

## 功能特性

- **自动检测构建文件** - 自动查找 `.hex` 和 `.bin` 文件
- **多种烧录模式** - 支持烧录、验证、复位
- **多调试器支持** - ST-Link V2, J-Link, CMSIS-DAP
- **跨平台** - 提供 Shell 和 Python 两种实现
- **错误处理** - 完善的错误提示和依赖检查

## 使用方法

### Windows (Python)

```bash
# 基本用法
python .claude-skills/embedded-flasher/scripts/flash.py

# 指定调试器
python .claude-skills/embedded-flasher/scripts/flash.py --debugger stlink-v2

# 仅烧录（不复位）
python .claude-skills/embedded-flasher/scripts/flash.py --mode flash

# 设置速度
python .claude-skills/embedded-flasher/scripts/flash.py --speed 2000

# 只构建不烧录
python .claude-skills/embedded-flasher/scripts/flash.py --build-only
```

### Linux (Shell)

```bash
# 基本用法
bash .claude-skills/embedded-flasher/scripts/flash.sh

# 指定调试器
bash .claude-skills/embedded-flasher/scripts/flash.sh -d stlink-v2

# 使用 J-Link
bash .claude-skills/embedded-flasher/scripts/flash.sh -d jlink

# 仅烧录
bash .claude-skills/embedded-flasher/scripts/flash.sh -m flash
```

### 配合 CMake 生成器

完整的开发流程：

```bash
# 1. 生成 CMake 配置
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py

# 2. 配置并构建
mkdir -p build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-toolchain.cmake
make -j4

# 3. 烧录固件
../.claude-skills/embedded-flasher/scripts/flash.py
```

## 参数说明

| 参数 | Shell | Python | 说明 | 默认值 |
|------|--------|--------|--------|
| 调试器 | `-d` | `--debugger` | `stlink-v2` |
| 目标芯片 | `-t` | `--target` | `stm32f1x` |
| 烧录模式 | `-m` | `--mode` | `reset` |
| 烧录速度 | `-s` | `--speed` | `1000` (kHz) |
| 仅构建 | `-b` | `--build-only` | `false` |
| 项目目录 | | `--project-dir` | `.` |
| 构建目录 | | `--build-dir` | `build` |

## 烧录模式

| 模式 | 说明 | OpenOCD 命令 |
|------|------|---------------|
| `flash` | 仅烧录 | `program file.hex verify` |
| `verify` | 烧录+验证 | `program file.hex verify` |
| `reset` | 烧录+验证+复位 | `program file.hex verify reset exit` |

## 支持的调试器

### ST-Link V2

```bash
# 自动配置
--debugger stlink-v2
```

配置文件：`interface/stlink-v2.cfg` + `target/stm32f1x.cfg`

传输设置：
```
transport select hla_adapter_kit
adapter speed 1000
```

### J-Link

```bash
# 使用 J-Link
--debugger jlink
```

配置文件：`interface/jlink.cfg` + `target/stm32f1x.cfg`

传输设置：
```
transport select jlink
adapter speed 1000
```

### CMSIS-DAP

```bash
# 通用调试器
--debugger cmsis-dap
```

配置文件：`interface/cmsis-dap.cfg` + `target/stm32f1x.cfg`

传输设置：
```
transport select cmsis-dap
adapter speed 1000
```

## 芯片系列对照

| 芯片系列 | OpenOCD 目标 |
|---------|---------------|
| STM32F0x | `stm32f0x` |
| STM32F1x | `stm32f1x` ✓ (默认) |
| STM32F2x | `stm32f2x` |
| STM32F3x | `stm32f3x` |
| STM32F4x | `stm32f4x` |
| STM32F7x | `stm32f7x` |
| STM32G0x | `stm32g0x` |
| STM32G4x | `stm32g4x` |
| STM32H7x | `stm32h7x` |
| STM32L0x | `stm32l0x` |
| STM32L4x | `stm32l4x` |

## 依赖要求

### 必需

- **OpenOCD** - Open On-Chip Debugger
  - Windows: 下载 `.exe` 可执行文件
  - Linux: `sudo apt install openocd`

- **调试器硬件**
  - ST-Link V2 / ST-Link
  - J-Link
  - CMSIS-DAP 兼容调试器

### 可选

- **ARM GCC 工具链** - 用于构建固件
- **CMake** - 用于项目配置

## 常见问题

### OpenOCD 找不到调试器

**症状：** `Error: couldn't find stlink` 或 `Error: couldn't find jlink`

**解决方案：**
1. 检查 USB 驱动是否安装
2. 确认调试器正确连接
3. Windows 下可能需要以管理员身份运行

### 烧录失败：Error: target not halted

**症状：** `Error: target not halted`

**原因：** 芯片正在运行保护代码，无法写入 Flash

**解决方案：**
1. 检查 BOOT0/BOOT1 引脚配置
2. 确认芯片未被 BOOT 模式锁定
3. 尝试降低烧录速度

### 烧录速度慢

**症状：** 烧录时间过长

**解决方案：** 调整速度参数
```bash
# 尝试更快的速度
flash.py --speed 2000

# 或者更慢但更稳定
flash.py --speed 500
```

### 构建文件不存在

**症状：** `未找到构建文件: build/Project.hex`

**解决方案：** 先运行 CMake 生成器并构建
```bash
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-toolchain.cmake
make -j4
```

## 示例输出

```
[INFO] === STM32 固件烧录工具 ===
[INFO] 找到 OpenOCD: openocd.exe
[INFO] 开始烧录: Project.hex
[INFO] 调试器: stlink-v2
[INFO] 目标: stm32f1x
[INFO] 速度: 1000 kHz
[INFO] 模式: reset

Open On-Chip Debugger 0.12.0
Licensed under GNU GPL v2 or later

adapter speed 1000
Info : stm32f1x.cpu: hardware has 6 breakpoints, 4 watchpoints
Info : Listening on port 3333 for gdb connections
program Project.hex verify reset exit
wrote 16384 bytes from file Project.hex in 0.8259s (19.828 KiB/s)
verified 16384 bytes in 0.097866s (167.583 KiB/s)
shutdown command invoked

[INFO] 烧录成功！
[INFO] 固件信息:
[INFO]   文件: Project.hex
[INFO]   大小: 16,384 字节 (16 KB)
```

## 目录结构

```
.claude-skills/embedded-flasher/
├── SKILL.md          # 主文档
├── README.md         # 使用说明
├── package.json       # 包配置
└── scripts/
    ├── flash.sh        # Shell 版本（Linux）
    └── flash.py        # Python 版本（Windows/Linux）
```

## 相关资源

- [OpenOCD 官网](https://openocd.org)
- [OpenOCD GitHub](https://github.com/openocd-org/openocd)
- [STM32 ST-Link](https://www.st.com/en/development-tools/stm32-st-link.html)

## 相关 Skills

- **embedded-cmake-generator** - STM32 项目 CMake 工程生成器
