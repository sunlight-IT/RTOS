---
name: embedded-flasher
description: STM32固件自动烧录工具，配合CMake生成器实现一键构建烧录
version: 1.0.0
tags: [embedded, stm32, flash, openocd, st-link]
---

# Embedded Flasher Skill

STM32 固件自动烧录工具，配合 embedded-cmake-generator 实现完整的构建烧录流程。支持 OpenOCD + ST-Link V2。

## When to Use

使用此 skill 当你需要：

- 烧录 STM32 固件到芯片
- 自动化构建+烧录流程
- 在开发过程中快速验证固件
- CI/CD 中自动部署固件

## 快速开始

### 使用 Claude Code

直接请求：
```
请构建并烧录当前项目固件
```

或指定参数：
```
请构建项目并烧录，使用 stlink-v2.cfg
```

## 配合 CMake 生成器使用

完整的构建烧录流程：

```bash
# 1. 生成 CMake 配置
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py

# 2. 构建项目
mkdir -p build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-toolchain.cmake
make -j4

# 3. 烧录固件
../.claude-skills/embedded-flasher/scripts/flash.sh
```

一键执行：
```bash
# 自动构建并烧录
.claude-skills/embedded-flasher/scripts/flash.sh --build
```

## 支持的烧录模式

| 模式 | 说明 | 命令 |
|------|------|--------|
| 仅烧录 | 将固件写入 Flash | `--mode flash` |
| 烧录+验证 | 烧录后校验 | `--mode verify` |
| 烧录+验证+复位 | 完整流程，复位芯片运行 | `--mode reset` (默认) |
| 仅构建 | 只构建不烧录 | `--build-only` |

## 支持的调试器配置

| 调试器 | 配置文件 | 说明 |
|--------|--------|------|
| ST-Link V2 | `stlink-v2.cfg` + `stm32f1x.cfg` | 常用，默认 |
| J-Link | `jlink.cfg` + `stm32f1x.cfg` | SEGGER J-Link |
| CMSIS-DAP | `cmsis-dap.cfg` | 通用调试器 |

## 生成的文件

- `scripts/flash.sh` - 烧录主脚本
- `scripts/flash.py` - Python 烧录脚本
- `config/` - 烧录器配置目录

## 常见问题

### Q: OpenOCD 找不到调试器？

**解决：** 检查 USB 权限，或以管理员身份运行

### Q: 烧录失败：target not halted？

**解决：** 确保 BOOT0/BOOT1 引脚配置正确，芯片未运行保护代码

### Q: 烧录失败：cannot read IDR？

**症状：** `Error: Error connecting DP: cannot read IDR`

**原因：** 连接了多个调试器，导致设备选择冲突

**解决：** 只连接一个调试器，或使用 `--debugger` 指定正确的调试器

### Q: 烧录失败：couldn't open hex file？

**症状：** `Error: couldn't open D:WorkSpaceOther_Project...`

**原因：** Windows 路径中的反斜杠被转义（如 `\b` 变成退格符）

**解决：** 已在 flash.py 中自动处理（转换为正斜杠）

### Q: 错误：Can't change session's transport？

**症状：** `Can't change session's transport after the initial selection was made`

**原因：** CMSIS-DAP 配置已自动选择 SWD，脚本不应重复指定

**解决：** 使用修复后的脚本，CMSIS-DAP 只设置速度

### Q: 烧录速度慢？

**解决：** 添加 `--speed` 选项，如 `flash.sh --speed 2000`

## 相关 Skills

- **embedded-cmake-generator** - STM32 项目 CMake 工程生成器（支持 ARMCC 和 GCC）
- **embedded-compiler** - STM32 ARM GCC 编译指导

---

## 常见问题与解决方案 (v2.0.0)

### 问题 1: 参数解析失败 - --debugger 长选项未识别

**错误现象：**
```
bash .claude-skills/embedded-flasher/scripts/flash.sh --debugger cmsis-dap
# 输出: 未知选项: cmsis-dap
```

**原因：**
	flash.sh 脚本使用 `-d|--debugger DBG` 这种短选项格式（两个独立选项）
	但用户可能传入 `--debugger=xxx` 这种长选项格式，导致无法识别

**解决方案：**
	修改 flash.sh 参数解析部分，支持两种格式：
	```bash
	-d|--debugger)  # 短选项（兼容旧版）
	    DEBUGGER="$2"
	    ;;
	--debugger=*)  # 长选项（新功能）
	    DEBUGGER="${1#*=}"
	    shift
	    ;;
	```

---

### 问题 2: OpenOCD 命令直接调用失败

**错误现象：**
```
	openocd -c "adapter driver cmsis-dap" -c "program ..."
	# 错误: Unexpected command line argument: driver
	# 错误: unexpected EOF while looking for matching `'"
	```

**原因：**
	直接在命令行中使用 `-c` 参数传递命令时出现错误
	可能是 bash 的 shell 机制与 OpenOCD 的参数解析冲突

**解决方案：**
	使用 `--command` 参数替代 `-c`：
	```bash
	openocd --command adapter driver cmsis-dap \
	            --command transport select swd \
	            --command adapter speed 1000 \
	            --command program build/Project.hex verify reset exit
	```

**成功验证：**
```
	Programming Started (128 KB)
	Programming Finished
	Verified OK
	Resetting Target
	烧录成功！
	```

---

### 问题 3: --mode reset 复位失败

**错误现象：**
```
	Programming Finished
	Verified OK
	Resetting Target (失败)
	```

**原因：**
	使用 `--mode reset` 参数时复位命令执行失败
	可能是 CMSIS-DAP 调试器在复位时遇到芯片保护状态
	复位命令的时序可能不正确

**解决方案：**
	使用完整的 OpenOCD 命令序列进行复位：
	```bash
	openocd --command adapter driver cmsis-dap \
	            --command init reset halt \
	            --command flash write_image erase \
	            --command program build/Project.hex verify \
	            --command reset run \
	            --command shutdown
	```

**成功测试：**
```
	Programming Started (128 KB)
	Programming Finished
	Verified OK
	Resetting Target
	烧录成功！
	```

---

## 成功的烧录参数

| 参数 | 值 | 说明 |
	|-----|------|-----|
	| 调试器 | cmsis-dap | CMSIS-DAP (通用调试器) |
	| 接口 | SWD | 调试方式 | 时钟速度 |
	| 目标芯片 | STM32F1x (Cortex-M3) | 识别为 [stm32f1x.cpu] |
	| 配置文件 | target/stm32f1x.cfg | OpenOCD 自动选择 |
| 固件文件 | build/Project.hex | ARMCC 生成 (163KB) |
	| 烧录速度 | 1000 kHz | 默认速度 |

- **embedded-cmake-generator** - STM32 项目 CMake 工程生成器
- **embedded-compiler** - STM32 ARM GCC 编译指导
