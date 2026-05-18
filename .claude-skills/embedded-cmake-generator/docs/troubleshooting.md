# 常见问题与解决方案

## 问题 1: 非标准散列文件路径

**现象：** `Error: L6236E: No section matches selector`

**原因：** 生成的散列文件路径默认为 `MDK-ARM/Project/Project.sct`，但实际文件在其他位置。

**解决：** 使用 `--keil-project` 指定 .uvprojx 文件，技能会自动提取散列文件路径。或设置 `embedded-cmake.json` 中的 `armcc_scatter_file`。

---

## 问题 2: 缺少关键的宏定义

**现象：** 大量 `identifier is undefined` 编译错误。

**原因：** CMakeLists.txt 中 `add_definitions()` 为空。非 CubeMX 项目的编译器宏（如 `__CPU_APM32`、`FSL_RTOS_UCOSII`）未被传递。

**解决：** 使用 Keil 项目文件（自动提取宏），或在 `embedded-cmake.json` 中配置 `defines` 数组。

---

## 问题 3: 多个 config.h 冲突

**现象：** 明明定义了 `__DEBUG_LEVEL`，仍然报未定义。

**原因：** 项目中存在多个 `config.h`（如 `Main/config.h` 和 `Common/ToolsEx/config.h`），include 路径顺序导致编译器先找到错误的文件。

**解决：** 技能自动检测并调整优先级，确保 Main/config.h 最高优先级。控制台会输出被遮蔽的 config.h 警告。

---

## 问题 4: 单片化 include 导致重复符号

**现象：** 链接时报重复符号错误，或某些文件编译报错。

**原因：** Keil 项目常使用 `#include "xxx.c"` 模式将多个 .c 文件合并为一个编译单元。CMake 默认将每个 .c 文件独立编译。

**解决：** 技能自动检测 `#include "*.c"` 模式（仅 Keil 项目），将叶子文件从编译列表中排除。控制台会列出所有被排除的文件。

---

## 问题 5: uCOS-II CPU 数据大小错误

**现象：** `error: "CPU_CFG_DATA_SIZE illegally #defined in 'cpu.h'"`

**原因：** ARMCC 预处理器在展开 uCOS-II 的 CPU 数据大小宏时，与预期值不匹配。

**解决：** 技能自动为 uCOS-II 项目添加 `-DCPU_CFG_DATA_SIZE=CPU_WORD_SIZE_32 -DCPU_CFG_DATA_SIZE_MAX=CPU_WORD_SIZE_64`。

---

## 问题 6: CMake 路径反斜杠转义

**现象：** `Invalid character escape '\A'`

**原因：** 自动检测的路径（如 `ARM-Cortex-M3\RealView`）包含反斜杠，CMake 将其解释为转义序列。

**解决：** 所有生成到 CMake 代码中的路径统一使用正斜杠。

---

## 问题 7: CubeMX 项目被误识别为 Keil 项目

**现象：** 源文件列表缺失（如 44 而非 88），芯片信息缺失。

**原因：** CubeMX 自动生成的 `MDK-ARM/*.uvprojx` 被优先检测。

**解决：** v4.0.1 已修复 — 检测到 `.ioc` 时自动跳过 Keil 检测路径。

---

## 问题 8: Cortex-Debug + OpenOCD 调试时 .data/.bss 未初始化 (v4.0.2)

**现象：**
使用 VS Code Cortex-Debug + OpenOCD 调试 CMake 构建的 ARMCC 项目时，FreeRTOS 在 `xTaskResumeAll` 中触发 `configASSERT` 死机。变量 `pxEnd`、`uxSchedulerSuspended` 等在 `main()` 处显示为 `0xFFFFFFFF` 而非 `0x00000000`。

**死机链路：**
```
vTaskSuspendAll → ++uxSchedulerSuspended
→ 0xFFFFFFFF + 1 = 0x00000000 (溢出)
→ xTaskResumeAll: configASSERT(0) → 断言失败 → CPU 死锁
```

**原因：**
ARMCC (armlink) 生成的 ELF 中，`RW_IRAM1` PROGBITS 段（.data 初始值）**不在 ELF PT_LOAD 段覆盖范围内**（`allocated section 'RW_IRAM1' not in segment`）。GDB `load` 命令通过 OpenOCD 加载 AXF 时：
1. ER_IROM1 段只写入 flash `0x08000000-0x0800E573`（RO 代码），**不包含** flash `0x0800E574+` 处的 156 字节数据初始值
2. RW_IRAM1 段向 RAM `0x20000000` 的写入**静默失败**
3. `__main` 从 flash 读取数据初始值 → 读到 `0xFFFFFFFF`（未写入区域）→ 复制到 RAM → `.data` 段全为 `0xFFFFFFFF`

**三种加载路径对比：**
| 加载方式 | Flash 数据初始值 | RAM .data 写入 | 结果 |
|----------|:---:|:---:|:---:|
| Keil ULINK | ✅ | ✅ | 正常 |
| OpenOCD `program` .hex | ✅ | 不写 (依赖 __main) | **正常** |
| GDB `load` .axf | ❌ | ❌ | **失败** |

**解决：**
在 VS Code `launch.json` 的 Cortex-Debug 配置中添加 `loadFiles` 指向 `.hex` 文件，绕过 GDB `load` 改用 OpenOCD `program` 命令：

```json
{
    "loadFiles": ["${workspaceFolder}/build/Project.hex"]
}
```

> **注意**：此问题仅影响 ARMCC (Keil MDK) 构建的 ELF + GDB `load` 组合。GCC 构建的 ELF 不依赖 scatter-loading 机制，不受此问题影响。

---

## 问题 9: VS Code Cortex-Debug 无法匹配源文件 / 断点无效 (v4.0.3)

**现象：**
使用 VS Code Cortex-Debug 调试 CMake 构建的 ARMCC .axf 时，无法在源码中设置断点，或断点显示为灰色。`call stack` 中的源文件链接无法打开正确文件。

**原因：**
CMake 总是向 armcc 传递绝对路径（如 `D:\WorkSpace\...\Core\Src\main.c`），导致 DWARF 调试信息中嵌入绝对路径。Cortex-Debug 在不同机器或不同路径下无法匹配源文件。

Keil MDK 编译时会传递相对路径（`../Core/Src/main.c`），`DW_AT_comp_dir` 指向 `MDK-ARM/` 目录，调试器通过组合 `comp_dir + DW_AT_name` 自动解析为正确路径。

**解决：**
v4.0.3 已自动修复 — 技能生成：
1. `armcc-toolchain.cmake` 中 `-g` 移至 `CMAKE_C_FLAGS_INIT`（始终生成 DWARF）
2. `armcc-relpath-wrapper.py` 编译包装器在调用 armcc 前将绝对路径转为相对路径
3. 路径使用正斜杠格式（`../Core/Src/main.c`），与 Keil MDK 一致

生成的 `.axf` 中 DWARF 源文件路径：
```
../Core/Src/main.c
../Middlewares/Third_Party/FreeRTOS/Source/tasks.c
```

而不再是：
```
D:\WorkSpace\...\Core\Src\main.c
```

---

## 历史问题 (v3.0 之前，已自动处理)

### CMake 测试链接失败

**错误：** `No section matches selector - no section to be FIRST/LAST.`

**原因：** CMake 在测试编译器时尝试链接简单程序，但使用了项目的 scatter file。

**解决：** armcc-toolchain.cmake 中已自动添加 `CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY`。

### fromelf 命令语法错误

**错误：** `Fatal error: Q3900U: Unrecognized option '--data'.`

**解决：** 生成器只使用 `--bin --output` 生成二进制文件。

### 输出文件名错误

链接器输出 `.o` 文件，fromelf 需要 `.axf` 文件。CMakeLists.txt 中已自动设置 `SUFFIX ".axf"`。

### Bash 路径分隔符

Windows 反斜杠在 bash 中无法正确解析。工具链文件已统一使用正斜杠路径。

### sysmem.c 链接符号不匹配

`_sbrk()` 函数需要根据编译器类型使用不同的符号定义。ARMCC 使用 scatter file 符号，GCC 使用 linker script 符号。

## 问题 9: 设备上电后不运行，调试无法跳转到 main

**症状：**
- Cortex-Debug 调试时程序停在复位向量处，不进入 main()
- 设备正常上电后无任何反应（LED 不亮、串口无输出）
- 构建 0 错误，.axf / .hex / .bin 均正常生成

**根因：**
ARMCC 链接时未使用 `--library_type=microlib`，导致 `__main` 使用标准 ARM C 库初始化流程。
标准库的 `__rt_entry` 链需要 `_platform_post_stackheap_init`、`__rt_lib_init` 等板级支持函数，
裸机项目不提供这些函数，程序在启动阶段卡死。

**验证方法：**
```bash
# 检查 map 文件中的库路径
grep "microlib\|clib" build/*.map
# 正常：../clib/microlib/...
# 异常：../clib/ 下出现 rtentry.o、libinit.o 等（标准库）
```

**解决方案：**
1. 确保 `armcc-toolchain.cmake` 中链接标志包含 `--library_type=microlib`
2. 确保汇编标志包含 `--pd "__MICROLIB SETA 1"`（启动文件条件编译依赖此符号）
3. 重新生成：`python scripts/generate-cmake.py --keil-project path/to/project.uvprojx`
4. 清理构建目录后重新编译：`rm -f build/CMakeCache.txt && cd build && cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/armcc-toolchain.cmake && make`

**关联：**
- Microlib vs 标准库启动流程对比见 [toolchain-reference.md](toolchain-reference.md)

---

## 问题 10: uGnu 编译选项未传递，GNU 扩展语法项目编译报错

**症状：**
- Keil 项目中使用了 GNU 扩展语法（如函数声明不兼容、结构体初始化器过多等）
- CMake ARMCC 构建时报大量 `#159: declaration is incompatible with previous`、`#146: too many initializer values` 等错误
- 同一个 .uvprojx 在 Keil MDK 中编译通过

**根因：**
uvprojx 中 `<uGnu>1</uGnu>`（启用 GNU 扩展模式）未被解析器检测到，生成器始终不传递 `--gnu` 给 ARMCC。ARMCC 在 strict C99 模式下比 Keil IDE 更严格地检查函数声明兼容性。

uvprojx XML 结构中的查找路径曾错误指向 `<VariousControls>`（兄弟节点），而非 `<Cads>`（父节点）：
```xml
<Cads>                          ← uGnu 在这里
  <uGnu>1</uGnu>
  <VariousControls>             ← 旧代码从这里找 uGnu（找不到）
    <Define>...</Define>
  </VariousControls>
</Cads>
```

**验证方法：**
```bash
# 检查 uvprojx 中是否有 uGnu=1
grep "<uGnu>1</uGnu>" path/to/project.uvprojx

# 检查生成的 armcc-toolchain.cmake 中是否有 --gnu
grep --gnu cmake/armcc-toolchain.cmake
```

**解决方案：**
重新生成 CMake 配置即可（v4.1.2+）：
```bash
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py --keil-project path/to/project.uvprojx
```

**影响范围：** 使用 GNU 扩展语法的 Keil 项目。CubeMX 项目（无 Keil uvprojx）不受影响。

---

## 问题 11: scan 排除列表无法移除默认项

**现象：**
`embedded-cmake.json` 中配置 `"exclude_dirs": ["build"]` 无法覆盖默认排除规则，`build/` 目录仍被排除。项目中的头文件（如 `build/F_WLM200_C_Standard_AppHal_APM32/CustApp/CustConfig.h`）无法被检测到。

**根因：**
`config.py` 的 `_merge_scan_config()` 默认只支持追加到列表，不支持移除：
```python
for item in data[key]:
    if item not in getattr(scan, key):
        getattr(scan, key).append(item)   # 只能追加
```
默认排除列表已有 `"build"`，用户无法通过配置移除。

**解决：**
v4.1.3+ 支持 `-` 前缀移除语法。在 `embedded-cmake.json` 中配置：
```json
{
  "scan": {
    "exclude_dirs": ["-build"]
  }
}
```
带 `-` 前缀的项会从默认排除列表中移除，不带前缀的项正常追加。

**注意：** 此语法适用于所有列表字段（`exclude_dirs`、`exclude_dir_patterns`、`exclude_files`、`exclude_file_patterns`、`extra_exclude_header_dirs`）。

**验证方法：**
```bash
# 生成 CMake 配置（启用详细日志）
python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py --dry-run
# 检查扫描器是否包含了原被排除目录下的文件
grep "CustConfig\|build/.*\.h" cmake/project_config.cmake
```

---

## 问题 12: uvprojx 中 .lib 预编译库文件路径解析失败

**现象：**
ARMCC 链接阶段大量符号未定义（如 `GSS_*`、`TOOLS_SnPrintEx`、`FIFO_*`、`LMEM_*`），这些符号实现在预编译 `.lib` 文件中但未被链接。

**根因：**
Keil uvprojx 中 `.lib` 文件使用相对路径（如 `../../Common/F9164_LP100_APM32_V.lib`），解析器默认相对 `.uvprojx` 文件目录解析。当 `.uvprojx` 位于深层子目录（如 `build/F_WLM200_C_Standard_AppHal_APM32/KeilPrj/`）时，`../../Common/` 解析到 `build/Common/`。如果该目录存在（如 SVN 原始项目），这能满足需要；但如果目录缺失（如仅拷贝了部分目录的 Git 仓库），`os.path.isfile()` 返回 `False`，lib 文件被静默跳过。

**解决：**
v4.1.3+ 增加项目根目录回退：当相对 `.uvprojx` 目录解析失败时，尝试相对项目根目录解析。同时确保构建环境完整，SVN 原始项目中的 `build/Common/` 目录包含所需的预编译库文件。

**验证方法：**
```bash
# 检查生成的 CMakeLists.txt 中是否包含 lib 链接
grep "target_link_libraries" build/CMakeLists.txt
# 应包含类似：${CMAKE_CURRENT_SOURCE_DIR}/build/Common/XXXX.lib

# 检查 lib 文件是否存在
ls -la build/Common/*.lib
```

---

## 问题 13: `_header_patterns.json` 位置错误导致 ChipDB 初始化崩溃

**现象：**
运行 `generate-cmake.py` 时抛出 `AttributeError: 'list' object has no attribute 'get'`，回溯指向 `chip_db.py` 的 `_parse_item()` 方法中 `data.get("cpu")` 调用。

**根因：**
`_header_patterns.json`（JSON 数组格式 `[{...}]`）被放置在 `data/chips/` 目录中，与芯片数据 JSON 文件（对象格式 `{...}`）同级。`JsonRegistry._load_from_paths()` 遍历目录下所有 `*.json` 文件，将 `_header_patterns.json` 的数组内容当作芯片数据对象解析，导致 `.get()` 在 list 上调用失败。

**解决：**
v4.1.3+ 将 `_header_patterns.json` 移至 `data/` 根目录（与 `chips/`、`toolchains/` 同级），并更新 `detector.py` 中对应的加载路径。

**注意：** 自定义 `embedded-cmake.json` 中的 `chip_header_patterns` 字段不受此问题影响，此问题仅影响芯片数据库内部使用。

**验证方法：**
```bash
# 确认文件位置正确
ls .claude-skills/embedded-cmake-generator/embedded_cmake/data/_header_patterns.json

# 确认芯片数据目录没有混杂非芯片 JSON
ls .claude-skills/embedded-cmake-generator/embedded_cmake/data/chips/*.json | head -5
# 应只包含芯片数据文件（如 stm32f1.json, apm32e1.json 等）
```
