cmake_minimum_required(VERSION 3.20)
project(Project C CXX ASM)

# 加载项目配置
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/project_config.cmake)

# 根据编译器类型选择 FreeRTOS port
if(CMAKE_C_COMPILER_ID MATCHES "ARMCC")
    # ARMCC 使用 RVDS port
    list(REMOVE_ITEM C_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c
    )
    list(APPEND C_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/RVDS/ARM_CM3/port.c
    )
endif()

# 添加包含目录
include_directories(${INCLUDE_DIRS})

# 定义宏
add_definitions(-DSTM32F103xB)
add_definitions(-DUSE_HAL_DRIVER)

# 创建可执行文件
add_executable(${PROJECT_NAME}.elf
    ${C_SOURCES}
    ${CPP_SOURCES}
    ${ASM_SOURCES}
)

# ARMCC 工具链特定配置
if(CMAKE_C_COMPILER_ID MATCHES "ARMCC")
    # ARMCC 使用 fromelf 和 scatter file
    # 设置输出文件后缀为 .axf
    set_target_properties(${PROJECT_NAME}.elf PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        OUTPUT_NAME ${PROJECT_NAME}
        SUFFIX ".axf"
    )

    # 使用 fromelf 生成 bin 和 hex 文件（注意：fromelf 使用 --i32 生成 hex）
    add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} --bin --output ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.axf
        COMMAND ${CMAKE_OBJCOPY} --i32 --output ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.axf
        COMMENT "Generating .bin and .hex files"
    )
else()
    # GCC 工具链
    # 设置链接脚本
    if(DEFINED LINKER_SCRIPT)
        target_link_options(${PROJECT_NAME}.elf PRIVATE
            -T${LINKER_SCRIPT}
        )
    endif()

    # 链接库
    target_link_libraries(${PROJECT_NAME}.elf
        c
        m
        nosys
    )

    # 使用 objcopy 生成 HEX 和 BIN 文件
    add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
        COMMENT "Generating .hex and .bin files"
    )

    # 设置输出目录
    set_target_properties(${PROJECT_NAME}.elf PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        OUTPUT_NAME ${PROJECT_NAME}
    )
endif()

# 打印配置信息
message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "Compiler ID: ${CMAKE_C_COMPILER_ID}")
message(STATUS "C Sources: ${C_SOURCES}")
message(STATUS "C++ Sources: ${CPP_SOURCES}")
message(STATUS "ASM Sources: ${ASM_SOURCES}")
message(STATUS "Include Dirs: ${INCLUDE_DIRS}")
