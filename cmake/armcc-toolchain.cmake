# ARMCC 工具链配置 (Keil MDK 5.4)
# 使用正斜杠路径避免 bash 路径问题

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# 工具路径 - 根据实际情况修改
set(CMAKE_C_COMPILER "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin/armcc.exe")
set(CMAKE_CXX_COMPILER "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin/armcc.exe")
set(CMAKE_ASM_COMPILER "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin/armasm.exe")
set(CMAKE_OBJCOPY "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin/fromelf.exe")
set(CMAKE_SIZE "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin/fromelf.exe")
set(CMAKE_LINKER "D:/APP/Keil/Keil5MDK5.4/ARM/ARMCC/bin/armlink.exe")

# 输出后缀
set(CMAKE_EXECUTABLE_SUFFIX .axf)
set(CMAKE_LINK_LIBRARY_SUFFIX .a)
set(CMAKE_STATIC_LIBRARY_SUFFIX .a)

# 避免链接测试时使用 scatter file（重要：否则 CMake 测试编译会失败）
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# CPU 配置 (STM32F103C8T6 - Cortex-M3)
set(CPU_FLAGS "--cpu=Cortex-M3")

# 编译选项
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

# 链接器选项 - 使用 scatter file
set(CMAKE_EXE_LINKER_FLAGS_INIT " ")
string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT "--scatter ${CMAKE_CURRENT_SOURCE_DIR}/MDK-ARM/Project/Project.sct --entry Reset_Handler --list --map --xref --callgraph --symbols --info sizes --info totals --info unused --info veneers")
string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT "--debug")

# 预定义
add_definitions(-DUSE_HAL_DRIVER -DSTM32F103xB -DNO_USE_ZTHREAD_CHECK)
