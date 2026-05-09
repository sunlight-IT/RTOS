# Embedded CMake Generator

> **v3.0.0** — Universal embedded CMake build system generator with config-driven architecture, multi-chip support, and auto-detection.

跨芯片、跨工具链的通用嵌入式 CMake 构建系统生成器。支持 **ARMCC (Keil MDK)**、**GCC (arm-none-eabi)**，自动检测 STM32F1/F4/F7/G0/H7 等芯片系列，通过 JSON 配置文件或自动检测实现零配置使用。

## 功能特性

- **自动检测**：零配置自动识别 CubeMX 项目、芯片型号、RTOS 类型、工具链
- **配置驱动**：通过 `embedded-cmake.json` 配置项目，支持所有参数定制
- **双工具链**：ARMCC（Keil MDK 5.4）和 GCC（arm-none-eabi）双支持
- **芯片数据库**：JSON 格式芯片定义，按需扩展（STM32F1 已内置）
- **工具链数据库**：JSON 格式工具链配置，CPU/FPU 标志自动解析
- **智能扫描**：可配置源文件/头文件扫描规则，自动排除模板和不需要的文件
- **向后兼容**：v2.x CLI 接口完全保留，无配置迁移

---

## 快速开始

```bash
# 零配置使用（自动检测项目）
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py

# 指定芯片
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py --chip-family STM32F1

# 指定工具链
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py --toolchain gcc

# 预览（不生成文件）
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py --dry-run

# 生成配置模板
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py --init
```

---

## 工具链对比

| 特性 | ARMCC | GCC |
|-----|-------|-----|
| 编译器 | armcc.exe (Keil MDK 5.4) | arm-none-eabi-gcc |
| 链接器 | armlink.exe + scatter file | ld + linker script |
| FreeRTOS | RVDS/ARM_CM3/port.c | GCC/ARM_CM3/port.c |
| 启动文件 | arm/startup_stm32f103xb.s | startup_stm32f103xb.s |
| 输出格式 | .axf, .bin | .elf, .hex, .bin |
| 转换工具 | fromelf --bin | objcopy -O ihex/binary |

---

## 自动排除规则

### 目录排除

| 目录 | 说明 |
|------|------|
| `build*` | 所有构建临时目录 |
| `.git`, `.vscode`, `.idea` | 版本控制和 IDE 目录 |
| `MDK-ARM`, `EWARM` | Keil 和 IAR 工程目录 |
| `Examples`, `Templates` | 示例和模板目录 |
| `CMSIS_RTOS` | CMSIS-RTOS V1（与 V2 API 冲突）|
| `RVDS` | ARM RealView 编译器专用 |

### 文件排除

| 文件类型 | 说明 |
|---------|------|
| `*template*` | 模板文件（不需要编译）|
| `heap_1.c`, `heap_2.c`, `heap_3.c`, `heap_5.c` | FreeRTOS 其他 heap 实现（与 heap_4.c 冲突）|
| `cmsis_os.c`, `cmsis_os1.c` | CMSIS-RTOS V1（与 V2 API 冲突）|
| `syscalls.c` | 标准库系统调用，裸机环境不需要 |
| `SEGGER_RTT_ASM_ARMv7M.S` | 汇编优化文件（C 编译器处理问题）|

---

## 生成的文件

```
Project/
├── CMakeLists.txt              # 主 CMake 配置文件（ARMCC/GCC双支持）
├── cmake/
│   ├── project_config.cmake   # 项目配置（源文件、头文件）
│   ├── armcc-toolchain.cmake # ARMCC 工具链配置
│   └── arm-none-eabi-toolchain.cmake  # GCC 工具链配置
└── build/                    # 构建输出目录
    ├── Project.axf           # ARMCC 可执行文件 (253KB)
    ├── Project.bin           # 二进制烧录文件 (58KB)
    ├── Project.htm           # HTML 映射文件 (314KB)
    └── Project.map           # 文本映射文件 (227KB)
```

---

## 常见问题与解决方案

### 问题 1: CMake 测试编译失败

**错误：**
```
No section matches selector - no section to be FIRST/LAST.
```

**原因：**
CMake 在测试编译器时尝试链接一个简单程序，但使用了项目的 scatter file，而测试程序没有包含预期的 RESET 节。

**解决方案：**
在 `armcc-toolchain.cmake` 中添加：
```cmake
# 避免链接测试时使用 scatter file（重要！）
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
```
fromelf.exe: Could not open file 'Project.elf': No such file
```

**原因：**
ARMCC 链接器默认输出 `.o` 文件，fromelf 需要 `.axf` 文件。

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
Error: D:\WorkSpace\...\Project.elf': No such file or directory
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
根据编译器类型使用不同的符号定义（详见 SKILL.md）：
```c
#ifdef __GNUC__
    extern uint32_t _end;
    extern uint8_t _estack;
#else
    extern uint32_t Image$$RW_IRAM1$$ZI$$Limit;
    const uint32_t _end = Image$$RW_IRAM1$$ZI$$Limit;
    const uint8_t *_estack = (const uint8_t *)0x20005400;
#endif
```

---

## 参数选项

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `-d, --project-dir` | 项目根目录 | 当前目录 |
| `-o, --output-dir` | CMake 文件输出目录 | cmake/ |
| `-n, --project-name` | 项目名称 | 自动检测 |
| `-t, --toolchain` | 默认工具链 (armcc/gcc) | armcc |

---

## 使用示例

### 基本用法

```bash
# 生成并构建（ARMCC）
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py
cd build && cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/armcc-toolchain.cmake
make -j4
```

### 指定项目名称

```bash
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py -n MyProject
```

### 指定输出目录

```bash
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py -o build/cmake
```

### 切换到 GCC 工具链

```bash
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py --toolchain gcc
cd build && cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-toolchain.cmake
make -j4
```

---

## 验证清单

使用 ARMCC CMake 构建前，验证：

- [ ] 工具链路径正确：`armcc.exe` 存在指定位置
- [ ] CMakeLists.txt 中 `armcc-toolchain.cmake` 路径正确
- [ ] CMakeLists.txt 中 `SUFFIX ".axf"` 已设置
- [ ] armcc-toolchain.cmake 中有 `CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY`
- [ ] sysmem.c 中有 ARMCC/GCC 条件编译
- [ ] FreeRTOS 使用 RVDS port 而不是 GCC port

---

## 目录结构

```
.claude-skills/embedded-cmake-generator/
├── scripts/
│   └── generate-cmake.py    # 主生成脚本 (支持 --toolchain 参数)
├── SKILL.md                 # 完整的 Skill 文档
└── README.md               # 本文件
```

---

## 注意事项

1. **工具链路径：** ARMCC 工具链配置需要根据实际安装路径修改 `armcc-toolchain.cmake` 中的路径
2. **scatter file：** 确保 `MDK-ARM/Project/Project.sct` 文件存在
3. **sysmem.c：** 需要支持 ARMCC/GCC 条件编译
4. **FreeRTOS port：** ARMCC 必须使用 RVDS port

---

## 相关文档

- [SKILL.md](SKILL.md) - 完整的 Skill 文档（包含所有问题和解决方案）
- [stm32-armcc-build](../stm32-armcc-build/README.md) - ARMCC CMake 完整构建指南
- [embedded-flasher](../embedded-flasher/README.md) - STM32 烧录工具

---

## 更新日志

- **v2.0.0 (2026-04-17)**: 添加 ARMCC/GCC 双工具链支持，修复所有已知问题
- **v1.1.0 (2026-04-17)**: 从 GCC 迁移到 ARMCC
