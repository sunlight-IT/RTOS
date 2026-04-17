# ARMCC 5.6 工具链配置 (匹配 Keil MDK)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# 使用正斜杠路径，兼容 bash
set(CMAKE_C_COMPILER "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin/armcc.exe")
set(CMAKE_CXX_COMPILER "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin/armcc.exe")
set(CMAKE_ASM_COMPILER "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin/armasm.exe")
set(CMAKE_OBJCOPY "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin/fromelf.exe")
set(CMAKE_SIZE "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin/fromelf.exe")
set(CMAKE_LINKER "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin/armlink.exe")

set(CMAKE_EXECUTABLE_SUFFIX_ASM .o)
set(CMAKE_EXECUTABLE_SUFFIX_C .o)
set(CMAKE_EXECUTABLE_SUFFIX_CXX .o)

# ARMCC 链接器默认输出 .elf 文件
set(CMAKE_EXECUTABLE_SUFFIX .axf)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# CPU 配置 (STM32F103C8T6 - Cortex-M3)
# 与 Keil MDK 相同
set(CPU_FLAGS "--cpu=Cortex-M3")

# ARMCC 编译选项 - 匹配 Keil MDK 配置
# Keil MDK: <uC99>1</uC99>, <Optim>4</Optim>, <uThumb>0</uThumb>
# 注意：string(APPEND 会直接追加，确保有正确的空格
set(CMAKE_C_FLAGS_INIT " ")
string(APPEND CMAKE_C_FLAGS_INIT "${CPU_FLAGS} --c99")
string(APPEND CMAKE_C_FLAGS_DEBUG_INIT "-Otime -g")
string(APPEND CMAKE_C_FLAGS_RELEASE_INIT "-Otime --g")

set(CMAKE_CXX_FLAGS_INIT " ")
string(APPEND CMAKE_CXX_FLAGS_INIT "${CPU_FLAGS} --cpp --no_exceptions")
string(APPEND CMAKE_CXX_FLAGS_DEBUG_INIT "-Otime -g")
string(APPEND CMAKE_CXX_FLAGS_RELEASE_INIT "-Otime --g")

set(CMAKE_ASM_FLAGS_INIT " ")
string(APPEND CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS}")

# ARMCC 链接器选项
set(CMAKE_EXE_LINKER_FLAGS_INIT " ")
string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT "--scatter ${CMAKE_CURRENT_SOURCE_DIR}/MDK-ARM/Project/Project.sct --entry Reset_Handler --list --map --xref --callgraph --symbols --info sizes --info totals --info unused --info veneers")
string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT "--debug")

# 设置输出文件格式为 ELF - 强制输出 .elf
set(CMAKE_EXECUTABLE_SUFFIX .axf)
set(CMAKE_LINK_LIBRARY_SUFFIX .a)
set(CMAKE_STATIC_LIBRARY_SUFFIX .a)

# 添加 Keil MDK 的预定义宏
add_definitions(-DUSE_HAL_DRIVER -DSTM32F103xB)

# 设置预定义，禁用CMSIS的栈溢出检测（与ARMCC5不兼容）
add_definitions(-DNO_USE_ZTHREAD_CHECK)
