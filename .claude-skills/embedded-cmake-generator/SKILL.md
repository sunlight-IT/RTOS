---
name: embedded-cmake-generator
description: 通用嵌入式项目CMake工程自动生成器 (配置驱动, 多芯片, 多工具链)
version: 3.0.0
tags: [embedded, stm32, cmake, build-system, cmake-generator, armcc, gcc, universal]
---

# Embedded CMake Generator Skill (v3.0)

通用嵌入式 CMake 构建系统生成器。通过自动检测或 JSON 配置文件描述项目，自动生成完整的 CMake 构建系统。

**v3.0 新特性：**
- 配置驱动架构（`embedded-cmake.json`）
- 芯片数据库（JSON，按需扩展）
- 工具链数据库（CPU/FPU 标志自动解析）
- 多芯片/多项目自动检测
- 零配置使用 + 全参数可定制

支持 **ARMCC (Keil MDK)** 和 **GCC (arm-none-eabi)** 双工具链，内置 STM32F1 芯片定义。

## When to Use

使用此 skill 当你需要：

- 为嵌入式项目生成 CMake 构建系统（STM32、GD32、NXP 等）
- 自动检测项目配置（芯片型号、RTOS、工具链）
- 定制项目扫描规则、编译器标志、输出格式
- 在 CI/CD 中使用 CMake 构建嵌入式项目

## 快速开始

### 使用脚本生成

```bash
# 生成CMake工程（ARMCC默认）
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py

# 指定工具链
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py --toolchain armcc
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py --toolchain gcc

# 查看帮助
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py --help
```

### 使用 Claude Code

直接请求：
```
请为当前项目生成ARMCC CMake工程配置
```

---

## 构建命令

生成CMake工程后，使用以下命令构建：

### ARMCC 工具链

```bash
# 创建构建目录
mkdir -p build && cd build

# 配置CMake（使用ARMCC工具链）
cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/armcc-toolchain.cmake

# 编译
make -j4

# 清理
make clean
```

### GCC 工具链

```bash
# 创建构建目录
mkdir -p build && cd build

# 配置CMake（使用GCC工具链）
cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-toolchain.cmake

# 编译
make -j4
```

---

## 生成的文件

- `CMakeLists.txt` - 主CMake配置文件（支持ARMCC/GCC双工具链）
- `cmake/armcc-toolchain.cmake` - ARMCC工具链配置
- `cmake/arm-none-eabi-toolchain.cmake` - GCC工具链配置
- `cmake/project_config.cmake` - 项目目录和源文件配置

---

## ARMCC vs GCC 工具链

### 工具链选择

脚本自动生成两个工具链配置文件，根据 `CMAKE_C_COMPILER_ID` 自动选择：

| 配置 | ARMCC | GCC |
|-----|-------|-----|
| 编译器 | armcc.exe | arm-none-eabi-gcc |
| 链接器 | armlink.exe + scatter file | ld + linker script |
| FreeRTOS port | RVDS/ARM_CM3 | GCC/ARM_CM3 |
| 启动文件 | arm/startup_stm32f103xb.s | startup_stm32f103xb.s |
| 输出格式 | .axf + .bin | .elf + .hex + .bin |
| 转换工具 | fromelf --bin | objcopy -O ihex/binary |

### 输出文件对比

| 文件 | ARMCC | GCC |
|-----|-------|-----|
| 可执行 | .axf (253KB) | .elf (~180KB) |
| 烧录 | .bin (~58KB) | .bin + .hex |
| 调试信息 | .htm (314KB) + .map | .map |

---

## ARMCC 工具链配置

### 关键配置点

```cmake
# 1. 使用正斜杠路径（避免bash路径问题）
set(CMAKE_C_COMPILER "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin/armcc.exe")

# 2. 设置输出后缀为 .axf
set(CMAKE_EXECUTABLE_SUFFIX .axf)

# 3. 避免链接测试时使用 scatter file（重要！）
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 4. 链接器使用 scatter file
string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT
    "--scatter ${CMAKE_CURRENT_SOURCE_DIR}/MDK-ARM/Project/Project.sct "
    "--entry Reset_Handler --list --map --xref --callgraph --symbols "
    "--info sizes --info totals --info unused --info veneers")
```

### FreeRTOS Port

ARMCC **必须**使用 RVDS port：
```
Middlewares/Third_Party/FreeRTOS/Source/portable/RVDS/ARM_CM3/port.c
```

GCC 使用 GCC port：
```
Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c
```

### 启动文件

ARMCC 使用 arm 子目录：
```
Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/arm/startup_stm32f103xb.s
```

GCC 使用根目录：
```
startup_stm32f103xb.s
```

---

## 常见问题与解决方案

### 问题 1: CMake 尝试链接测试程序失败

**错误：**
```
No section matches selector - no section to be FIRST/LAST.
```

**原因：**
CMake 在测试编译器时尝试链接一个简单程序，但使用了项目的 scatter file，而测试程序没有包含预期的 RESET 节。

**解决方案：**
在 armcc-toolchain.cmake 中添加：
```cmake
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
```

---

### 问题 2: fromelf 命令语法错误

**错误：**
```
Fatal error: Q3900U: Unrecognized option '--data'.
```

**原因：**
fromelf 不支持 `--text --data --bss` 这样的参数组合。

**解决方案：**
只使用 `--bin --output` 生成二进制文件：
```cmake
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} --bin --output ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.axf
    COMMENT "Generating .bin file"
)
```

---

### 问题 3: 输出文件名错误

**错误：**
链接器输出 `.o` 文件，fromelf 需要 `.axf` 文件。

**解决方案：**
在 CMakeLists.txt 中设置 SUFFIX 属性：
```cmake
set_target_properties(${PROJECT_NAME}.elf PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    OUTPUT_NAME ${PROJECT_NAME}
    SUFFIX ".axf"
)
```

---

### 问题 4: Bash 路径分隔符问题

**错误：**
```
Error: D:\path\to\file': No such file or directory
```

**原因：**
Windows 反斜杠在 bash 中无法正确解析。

**解决方案：**
工具链文件使用正斜杠路径：
```cmake
set(CMAKE_C_COMPILER "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin/armcc.exe")
```

---

### 问题 5: sysmem.c 链接符号不匹配

**错误：**
```
undefined reference to `Image$$RW_IRAM1$$ZI$$Limit'
```

**原因：**
sysmem.c 中的 `_sbrk()` 函数使用了 ARMCC scatter file 特定的符号，而 GCC 使用 linker script 符号。

**解决方案：**
根据编译器类型使用不同的符号定义：
```c
void *_sbrk(ptrdiff_t incr)
{
#ifdef __GNUC__
    extern uint32_t _end;
    extern uint8_t _estack;
    extern uint32_t _Min_Stack_Size;
#else
    extern uint32_t Image$$RW_IRAM1$$ZI$$Limit;
    const uint32_t _end = Image$$RW_IRAM1$$ZI$$Limit;
    const uint8_t *_estack = (const uint8_t *)0x20005400;
    const uint32_t _Min_Stack_Size = 0x400;
#endif()
    // ...
}
```

---

## 参数说明

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `-d, --project-dir DIR` | 项目根目录 | 当前目录 |
| `-o, --output-dir DIR` | CMake文件输出目录 | cmake/ |
| `-n, --project-name NAME` | 项目名称 | 自动检测 |
| `-t, --toolchain {armcc,gcc}` | 默认工具链 | armcc |

---

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
| 目录 | `RVDS` | ARMCC 自动处理 |
| 文件 | `heap_1/2/3/5.c` | 与 heap_4.c 冲突 |
| 文件 | `cmsis_os.c`, `cmsis_os1.c` | V1 与 V2 冲突 |
| 文件 | `syscalls.c` | 标准库系统调用，裸机不需要 |
| 文件 | `SEGGER_RTT_ASM_ARMv7M.S` | C 编译器处理问题 |

---

## 验证清单

使用 ARMCC CMake 构建前，验证：

- [ ] 工具链路径正确：`armcc.exe` 存在指定位置
- [ ] CMakeLists.txt 中 `armcc-toolchain.cmake` 路径正确
- [ ] CMakeLists.txt 中 `SUFFIX ".axf"` 已设置
- [ ] sysmem.c 中有 ARMCC/GCC 条件编译
- [ ] FreeRTOS 使用 RVDS port 而不是 GCC port
- [ ] armcc-toolchain.cmake 中有 `CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY`

---

## 相关 Skills

- **[embedded-flasher](../embedded-flasher/README.md) - STM32 烧录工具**

---

## 更新日志

- **2026-04-17 v2.0.0**: 添加 ARMCC/GCC 双工具链支持，修复所有已知问题
- **2026-04-17 v1.1.0**: 从 GCC 迁移到 ARMCC
