# Embedded CMake Generator 使用说明

## 功能特性

本生成器自动为 STM32 嵌入式项目创建完整的 CMake 构建系统，包括：

- **自动扫描**：递归扫描项目目录，收集所有源文件和头文件路径
- **智能排除**：自动排除不需要的文件和目录
- **工具链配置**：生成 ARM GCC 工具链配置文件
- **多输出格式**：自动生成 .elf、.hex、.bin 文件

## 自动排除规则

### 目录排除

| 目录 | 说明 |
|------|------|
| `build*` | 所有构建临时目录 |
| `.git`, `.vscode`, `.idea` | 版本控制和 IDE 目录 |
| `MDK-ARM`, `EWARM` | Keil 和 IAR 工程目录 |
| `RVDS` | ARM RealView 编译器专用（与 GCC 不兼容）|
| `Examples`, `Templates` | 示例和模板目录 |
| `CMSIS_RTOS` | CMSIS-RTOS V1（与 V2 API 冲突）|

### 文件排除

| 文件类型 | 说明 |
|---------|------|
| `*template*` | 模板文件（不需要编译）|
| `heap_1.c`, `heap_2.c`, `heap_3.c`, `heap_5.c` | FreeRTOS 其他 heap 实现（与 heap_4.c 冲突）|
| `cmsis_os.c`, `cmsis_os1.c` | CMSIS-RTOS V1（与 V2 冲突）|

## 使用方法

### 基本用法

```bash
# 在项目根目录运行
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py
```

### 指定参数

```bash
# 指定项目目录
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py -d /path/to/project

# 指定输出目录
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py -o output

# 指定项目名称
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py -n MyProject
```

## 构建流程

```bash
# 1. 生成 CMake 配置
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py

# 2. 创建构建目录并配置
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-toolchain.cmake

# 3. 编译
make -j4

# 4. 输出文件
# build/Project.elf  - ELF 格式（调试用）
# build/Project.hex  - Intel HEX 格式（烧录用）
# build/Project.bin  - 二进制格式（烧录用）
```

## 生成的文件结构

```
Project/
├── CMakeLists.txt                    # 主 CMake 配置
├── cmake/
│   ├── arm-none-eabi-toolchain.cmake  # 工具链配置
│   └── project_config.cmake            # 项目配置（源文件和头文件列表）
└── build/                            # 构建目录（运行 cmake 后创建）
    ├── CMakeFiles/
    ├── Project.elf
    ├── Project.hex
    └── Project.bin
```

## 常见问题

### Q: 如何修改 CPU 配置？

编辑 `cmake/arm-none-eabi-toolchain.cmake`，修改 `CPU_FLAGS`：

```cmake
# Cortex-M3 (STM32F103)
set(CPU_FLAGS "-mcpu=cortex-m3 -mthumb -mfloat-abi=soft")

# Cortex-M4 (STM32F407)
# set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16")
```

### Q: 如何添加自定义编译选项？

在 `CMakeLists.txt` 中添加：

```cmake
add_compile_options(-Dmy_DEFINE)
add_compile_options(-Wno-error)
```

### Q: 如何排除特定源文件？

编辑 `cmake/project_config.cmake`，从源文件列表中移除不需要的文件。

### Q: 构建失败怎么办？

1. 检查工具链路径是否正确
2. 清理构建目录：`rm -rf build && mkdir build && cd build`
3. 查看 SKILL.md 中的"常见问题与解决方案"章节

## 依赖要求

- Python 3.6+
- CMake 3.20+
- ARM GCC 工具链（arm-none-eabi-gcc）
- Make 或 Ninja
