---
name: embedded-cmake-generator
description: STM32项目CMake工程自动生成器
version: 1.0.0
tags: [embedded, stm32, cmake, build-system, cmake-generator]
---

# Embedded CMake Generator Skill

自动为STM32嵌入式项目生成完整的CMake构建系统，通过扫描项目目录自动收集源文件和头文件路径。

## When to Use

使用此 skill 当你需要：

- 将现有STM32项目迁移到CMake构建系统
- 从头创建新的STM32 CMake工程
- 自动维护项目中的源文件列表
- 在CI/CD中使用CMake构建STM32项目

## 快速开始

在项目根目录下运行：

```bash
# 生成CMake工程（自动扫描当前目录）
./.claude-skills/embedded-cmake-generator/scripts/generate-cmake.sh

# 查看帮助
./.claude-skills/embedded-cmake-generator/scripts/generate-cmake.sh --help
```

### 使用 Claude Code

直接请求：
```
请为当前项目生成CMake工程配置
```

## 构建命令

生成CMake工程后，使用以下命令构建：

```bash
# 创建构建目录
mkdir -p build
cd build

# 配置CMake
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-toolchain.cmake

# 编译
make -j$(nproc)

# 清理
make clean
```

## 生成的文件

- `CMakeLists.txt` - 主CMake配置文件
- `cmake/arm-none-eabi-toolchain.cmake` - 工具链配置
- `cmake/project_config.cmake` - 项目目录和源文件配置

## 参数说明

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `--project-dir DIR` | 项目根目录 | 当前目录 |
| `--output-dir DIR` | CMake文件输出目录 | cmake/ |
| `--project-name NAME` | 项目名称 | 自动检测 |

## 扫描规则

默认扫描以下目录并排除 `build/`, `.git/`, `MDK-ARM/` 等：

```
Core/Src/    Core/Inc/
Drivers/
Middlewares/
Usr/
```

**自动排除的文件和目录：**

| 类型 | 路径 | 原因 |
|------|------|------|
| 目录 | `build*` | 构建临时目录 |
| 目录 | `RVDS` | ARM RealView 编译器专用 |
| 目录 | `CMSIS_RTOS` | 与 V2 API 冲突 |
| 文件 | `heap_1/2/3/5.c` | 与 heap_4.c 冲突 |
| 文件 | `cmsis_os.c` | V1 API 与 V2 冲突 |
| 文件 | `cmsis_os1.c` | V1 wrapper 与 V2 冲突 |
| 文件 | `*template*` | 模板文件 |

## 常见问题与解决方案

### 1. 编译错误：`stray '#' in program`

**错误信息：**
```
error: stray '#' in program
mov r0, #0
```

**原因：** RVDS 编译器的汇编文件被 GCC 编译

**解决方案：** 生成器已自动排除 `RVDS/` 目录，无需手动处理

---

### 2. 链接错误：`multiple definition of vPortFree`

**错误信息：**
```
error: multiple definition of `vPortFree'; first defined here
```

**原因：** 多个 FreeRTOS heap 实现被同时链接

**解决方案：** 生成器已自动排除 heap_1/2/3/5.c，只保留 heap_4.c

---

### 3. 类型冲突：`conflicting types for osMutexDelete`

**错误信息：**
```
error: conflicting types for 'osMutexDelete'
```

**原因：** CMSIS-RTOS V1 和 V2 头文件同时包含

**解决方案：** 生成器已自动排除 `CMSIS_RTOS` 目录，只使用 V2

---

### 4. 编译器参数错误：`cortex-m3;-mthumb`

**错误信息：**
```
error: unrecognized command line option '-mcpu=cortex-m3;-mthumb'
```

**原因：** Windows 下 CMake 将列表转为分号分隔字符串

**解决方案：** 生成器已使用单行字符串格式定义 `CPU_FLAGS`

---

### 5. 重定义警告：`USE_RTOS redefined`

**警告信息：**
```
warning: "USE_RTOS" redefined
```

**原因：** HAL 配置文件已定义该宏

**解决方案：** CMakeLists.txt 不再添加 `-DUSE_RTOS`

---

### 6. objcopy 输出路径错误

**错误信息：**
```
objcopy: cannot open 'Project.elf$<TARGET_FILE:Project.elf>': No such file
```

**原因：** `$<TARGET_FILE:...>` 生成器表达式位置错误

**解决方案：** 生成器已修正为 `$<TARGET_FILE:Project.elf> Project.hex`

## 相关 Skills

- **embedded-compiler** - STM32 ARM GCC 编译指导
- **embedded-flasher** - STM32 OpenOCD 固件烧录
