cmake_minimum_required(VERSION 3.20)
project(Project C CXX ASM)

# 工具链配置 - 强制使用ARMCC
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/armcc-toolchain.cmake CACHE STRING "" FORCE)

# ARMCC 使用 RVDS FreeRTOS port
if(CMAKE_C_COMPILER_ID MATCHES "ARMCC")
    # 替换 GCC port 为 RVDS port
    list(REMOVE_ITEM C_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c
    )
    list(APPEND C_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/RVDS/ARM_CM3/port.c
    )
    # 替换 GCC include 为 RVDS include
    list(REMOVE_ITEM INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3
    )
    list(APPEND INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/RVDS/ARM_CM3
    )
endif()

# 加载项目配置
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/project_config.cmake)

# ARMCC 使用 RVDS FreeRTOS port
if(CMAKE_C_COMPILER_ID MATCHES "ARMCC")
    # 替换 GCC port 为 RVDS port
    list(REMOVE_ITEM C_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c
    )
    list(APPEND C_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/RVDS/ARM_CM3/port.c
    )
    # 替换 GCC include 为 RVDS include
    list(REMOVE_ITEM INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3
    )
    list(APPEND INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/RVDS/ARM_CM3
    )
    # 替换 GCC startup 为 ARMCC startup
    list(REMOVE_ITEM ASM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/startup_stm32f103xb.s
    )
    list(APPEND ASM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/arm/startup_stm32f103xb.s
    )
    # 使用 ARMCC scatter file
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/MDK-ARM/Project/Project.sct")
endif()

# 添加包含目录
include_directories(${INCLUDE_DIRS})

# 定义宏（HAL配置文件中已定义USE_RTOS，此处不重复定义）
add_definitions(-DSTM32F103xB)
add_definitions(-DUSE_HAL_DRIVER)

# 定义编译选项
if(CMAKE_C_COMPILER_ID MATCHES "ARMCC")
    # ARMCC C/C++ 编译选项
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:--split_sections>)
else()
    # GCC 编译选项
    add_compile_options(
        -Wall
        -Wextra
        $<$<CONFIG:DEBUG>:-O0>
        $<$<CONFIG:RELEASE>:-O2>
        -g
        -fdata-sections
        -ffunction-sections
    )
endif()

# 创建可执行文件
add_executable(${PROJECT_NAME}.elf
    ${C_SOURCES}
    ${CPP_SOURCES}
    ${ASM_SOURCES}
)

# 设置链接脚本
if(DEFINED LINKER_SCRIPT)
    if(CMAKE_C_COMPILER_ID MATCHES "ARMCC")
        target_link_options(${PROJECT_NAME}.elf PRIVATE
            --scatter ${LINKER_SCRIPT}
        )
    else()
        target_link_options(${PROJECT_NAME}.elf PRIVATE
            -T${LINKER_SCRIPT}
        )
    endif()
endif()

# 链接库 (GCC 需要，ARMCC 不需要)
if(NOT CMAKE_C_COMPILER_ID MATCHES "ARMCC")
    target_link_libraries(${PROJECT_NAME}.elf
        c
        m
        nosys
    )
endif()

# 后处理：生成HEX和BIN文件
if(CMAKE_C_COMPILER_ID MATCHES "ARMCC")
    # ARMCC 使用 fromelf
    add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} --bin --output ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf
        COMMAND ${CMAKE_SIZE} --text ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf
        COMMENT "Generating .bin file and size info"
    )
else()
    # GCC 使用 objcopy
    add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
        COMMAND ${CMAKE_SIZE} ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf
        COMMENT "Generating .hex and .bin files"
    )
endif()

# 设置输出目录和文件后缀
set_target_properties(${PROJECT_NAME}.elf PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    OUTPUT_NAME ${PROJECT_NAME}
    SUFFIX ".elf"
)

# 打印配置信息
message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "C Sources: ${C_SOURCES}")
message(STATUS "C++ Sources: ${CPP_SOURCES}")
message(STATUS "ASM Sources: ${ASM_SOURCES}")
message(STATUS "Include Dirs: ${INCLUDE_DIRS}")
