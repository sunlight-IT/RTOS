# 工具链参考

## ARMCC vs GCC 对比

| 配置 | ARMCC | GCC |
|-----|-------|-----|
| 编译器 | armcc.exe | arm-none-eabi-gcc |
| 链接器 | armlink.exe + scatter file | ld + linker script |
| FreeRTOS port | RVDS/ARM_CM3 | GCC/ARM_CM3 |
| uCOS-II port | RealView/ARM-Cortex-M3 | GNU/ARM-Cortex-M3 |
| 启动文件 | arm/startup_*.s (CMSIS Templates) | startup_*.s (项目根目录) |
| 输出格式 | .axf + .bin | .elf + .hex + .bin |
| 转换工具 | fromelf --bin | objcopy -O ihex/binary |
| CMAKE_C_COMPILER_ID | ARMCC | GNU |

## ARMCC 关键配置

```cmake
# 1. 使用正斜杠路径（避免bash路径问题）
set(CMAKE_C_COMPILER "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin/armcc.exe")

# 2. 设置输出后缀为 .axf
set(CMAKE_EXECUTABLE_SUFFIX .axf)

# 3. 避免链接测试时使用 scatter file（重要！）
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 4. 链接器使用 scatter file
string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT
    "--scatter ${CMAKE_CURRENT_SOURCE_DIR}/path/to/Project.sct "
    "--entry Reset_Handler --list --map --xref --callgraph --symbols "
    "--info sizes --info totals --info unused --info veneers")
```

## FreeRTOS Port 路径

ARMCC **必须**使用 RVDS port：
```
Middlewares/Third_Party/FreeRTOS/Source/portable/RVDS/ARM_CM3/port.c
```

GCC 使用 GCC port：
```
Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c
```

> CPU 架构目录（ARM_CM3/ARM_CM4F 等）由芯片数据库自动选择。

## 启动文件路径

ARMCC（CMSIS Templates 子目录）：
```
Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/arm/startup_stm32f103xb.s
```

GCC（项目根目录）：
```
startup_stm32f103xb.s
```

## 构建前验证清单

- [ ] 工具链路径正确：编译器存在于指定位置
- [ ] CMakeLists.txt 中 `armcc-toolchain.cmake` 路径正确
- [ ] ARMCC 输出设置 `SUFFIX ".axf"`
- [ ] sysmem.c 中有 ARMCC/GCC 条件编译
- [ ] FreeRTOS port 与工具链匹配（ARMCC→RVDS, GCC→GCC）
- [ ] `armcc-toolchain.cmake` 中有 `CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY`
- [ ] uCOS-II 项目自动添加 `CPU_CFG_DATA_SIZE` 宏
- [ ] ARMCC `--library_type=microlib` + `--pd "__MICROLIB SETA 1"` 已配置

## Microlib vs 标准 ARM C 库启动流程

| 阶段 | Microlib | 标准 C 库 |
|------|----------|-----------|
| Reset_Handler | 设置 SP，跳转 __main | 同左 |
| __main | 直接拷贝 RW 数据到 RAM，清零 ZI | 调用 `__rt_entry` 链 |
| __rt_entry | — | 遍历 region table，调用 `__rt_entry_sh` |
| 板级初始化 | — | 需要 `_platform_post_stackheap_init` |
| 库初始化 | — | 需要 `__rt_lib_init` |
| 跳转 main | 直接 BX | 通过 `__rt_entry_main` 调用 |
| 裸机可用 | ✅ | ❌（缺少板级支持） |
| ROM 占用 | ~1KB | ~8KB+ |
