# 扫描规则

## 默认扫描行为

生成器默认递归扫描项目目录，收集所有 C/C++/ASM 源文件和头文件目录。排除规则通过 `embedded-cmake.json` 中的 `scan` 字段配置。

## 默认排除目录

扫描器**自动检测并排除 CMake 构建目录**（包含 `CMakeCache.txt` 的目录），无论目录名称如何。

| 目录 | 原因 |
|------|------|
| **CMake 构建目录** | 自动检测 `CMakeCache.txt`（防止 CMakeCCompilerId 等测试文件被编译） |
| `build*` | 构建临时目录（名称匹配） |
| `.git`, `.vscode`, `.idea` | 版本控制和 IDE 配置 |
| `MDK-ARM`, `EWARM` | Keil/IAR 工程目录 |
| `Template`, `Templates`, `Examples` | 模板和示例 |
| `Core_A`, `DSP`, `DSP_Lib_TestSuite`, `NN`, `RTOS2` | 非核心 CMSIS |
| `RVDS` | ARMCC port（自动条件处理） |
| `CMSIS_RTOS` | CMSIS-RTOS V1（自动使用 V2） |

## 默认排除文件

| 文件 | 原因 |
|------|------|
| `heap_1/2/3/5.c` | 与 heap_4.c 冲突 |
| `cmsis_os.c`, `cmsis_os1.c` | CMSIS-RTOS V1（与 V2 冲突） |
| `syscalls.c` | 标准库系统调用，裸机不需要 |
| `SEGGER_RTT_ASM_ARMv7M.S` | C 编译器处理问题 |
| `*template*`（模式） | 模板文件 |

## 自定义扫描规则

在项目根目录创建 `embedded-cmake.json`：

```json
{
  "scan": {
    "source_extensions": [".c", ".cpp", ".s", ".S", ".asm"],
    "exclude_dirs": ["build", ".git", "my_temp"],
    "exclude_dir_patterns": ["build*", "temp_*"],
    "exclude_files": ["unused_module.c"],
    "exclude_file_patterns": ["*_test.c", "*_bak.c"],
    "extra_exclude_header_dirs": ["third_party"]
  }
}
```

## 特殊检测

- **config.h 冲突检测**：项目存在多个 `config.h` 时，自动将 Main/config.h 优先级置顶
- **单片化 include 检测**（仅 Keil 项目）：自动识别 `#include "*.c"` 模式，排除叶子文件
- **预编译库检测**：自动从 Keil .uvprojx 提取 `.lib` 文件并链接
