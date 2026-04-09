# Embedded Skills 集成使用指南

本项目包含两个配套的 STM32 开发工具，实现完整的构建烧录流程。

**使用 Claude Code 时可以直接请求：** "请帮我构建并烧录当前项目固件"

## Skills 概览

| Skill | 功能 | 位置 |
|-------|------|--------|
| **embedded-cmake-generator** | CMake 工程自动生成器 | `embedded-cmake-generator/` |
| **embedded-flasher** | 固件自动烧录工具 | `embedded-flasher/` |

## 完整开发流程

### 方法一：分步执行

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

### 方法二：一键构建烧录

创建自定义脚本：

```bash
#!/bin/bash
# build-and-flash.sh

echo "=== 构建并烧录 STM32 固件 ==="

# 生成 CMake 配置
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py

# 配置并构建
mkdir -p build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-toolchain.cmake
make -j4

# 返回项目根目录
cd ..

# 烧录固件
python .claude-skills/embedded-flasher/scripts/flash.py
```

### 方法三：使用 Claude Code

直接请求：
```
请帮我构建并烧录当前项目的固件
```

Claude 会自动：
1. 调用 cmake 生成器
2. 配置并构建项目
3. 执行烧录

## 常用命令组合

### 仅重新生成配置

```bash
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py
```

### 仅重新构建

```bash
cd build && make -j4
```

### 仅烧录（不构建）

```bash
python .claude-skills/embedded-flasher/scripts/flash.py
```

### 快速烧录（不复位）

```bash
python .claude-skills/embedded-flasher/scripts/flash.py --mode flash
```

### 使用 J-Link 烧录

```bash
python .claude-skills/embedded-flasher/scripts/flash.py --debugger jlink
```

### 调整烧录速度

```bash
# 更快的速度
python .claude-skills/embedded-flasher/scripts/flash.py --speed 2000

# 更慢但更稳定
python .claude-skills/embedded-flasher/scripts/flash.py --speed 500
```

## 支持的 STM32 系列

| 系列 | OpenOCD 目标 | 烧录器 |
|------|---------------|---------|
| STM32F0x | `stm32f0x` | ST-Link, J-Link |
| STM32F1x | `stm32f1x` | ST-Link, J-Link |
| STM32F2x | `stm32f2x` | ST-Link, J-Link |
| STM32F3x | `stm32f3x` | ST-Link, J-Link |
| STM32F4x | `stm32f4x` | ST-Link, J-Link |
| STM32F7x | `stm32f7x` | ST-Link |
| STM32G0x | `stm32g0x` | ST-Link, J-Link |
| STM32G4x | `stm32g4x` | ST-Link, J-Link |
| STM32H7x | `stm32h7x` | ST-Link |
| STM32L0x | `stm32l0x` | ST-Link, J-Link |
| STM32L4x | `stm32l4x` | ST-Link, J-Link |

## 项目示例输出

### CMake 生成器输出

```
[INFO] 项目目录: /path/to/Project
[INFO] 输出目录: cmake
[INFO] 项目名称: Project
[INFO] 扫描C源文件...
[INFO] 扫描头文件目录...
[INFO] 启动文件: startup_stm32f103xb.s
[INFO] 链接脚本: STM32F103XX_FLASH.ld
[INFO] 找到: 94 个C文件, 0 个C++文件, 2 个汇编文件, 16 个头文件目录
[INFO] 生成项目配置文件...
[INFO] 生成工具链配置文件...
[INFO] 生成主CMakeLists.txt...
[INFO] CMake工程生成完成！

[INFO] 自动排除：
  - RVDS/ 目录（ARM RealView 编译器专用）
  - CMSIS_RTOS V1（使用 V2 API）
  - heap_1/2/3/5.c（使用 heap_4.c）
  - build* 目录（构建临时文件）

[INFO] 下一步：
  1. 创建构建目录: mkdir -p build && cd build
  2. 配置CMake: cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-toolchain.cmake
  3. 编译项目: make -j4
```

### 烧录器输出

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
.claude-skills/
├── README.md                          # 本文件
├── embedded-cmake-generator/           # CMake 生成器
│   ├── SKILL.md
│   ├── README.md
│   ├── package.json
│   └── scripts/
│       ├── generate-cmake.sh
│       └── generate-cmake.py
└── embedded-flasher/                   # 烧录器
    ├── SKILL.md
    ├── README.md
    ├── package.json
    └── scripts/
        ├── flash.sh
        └── flash.py
```

## 依赖安装

### Windows

1. **安装 Python** (如未安装)
   - 下载: https://www.python.org/downloads/
   - 勾选 "Add Python to PATH"

2. **安装 OpenOCD**
   - 下载: https://github.com/openocd-org/openocd/releases
   - 解压到可访问路径

3. **安装 ARM GCC 工具链**
   - 下载: https://developer.arm.com/downloads/-/gnu-rm
   - 解压并添加到 PATH

### Linux

```bash
# 安装 OpenOCD
sudo apt update
sudo apt install openocd

# 安装 ARM GCC 工具链
sudo apt install gcc-arm-none-eabi

# 将当前用户加入 dialout 组（解决 USB 权限问题）
sudo usermod -a -G dialout $USER
```

## 故障排除

### 问题：OpenOCD 权限错误

**Windows:**
- 以管理员身份运行终端
- 检查 USB 驱动是否正确安装

**Linux:**
- 将用户加入 `dialout` 组
- 创建 udev 规则

### 问题：CMake 配置错误

- 确保使用 `-G "MinGW Makefiles"` (Windows) 或 `-G "Unix Makefiles"` (Linux)
- 确保工具链文件路径正确

### 问题：构建失败

- 检查 ARM GCC 工具链是否在 PATH 中
- 确认源文件没有被修改导致语法错误
- 查看 `embedded-cmake-generator/SKILL.md` 中的常见问题

## 相关资源

- [OpenOCD 官网](https://openocd.org)
- [STM32 官网](https://www.st.com)
- [ARM Developer](https://developer.arm.com)
