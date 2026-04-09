cmake_minimum_required(VERSION 3.20)
project(Project C CXX ASM)

# 工具链配置
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/arm-none-eabi-toolchain.cmake CACHE STRING "" FORCE)

# 加载项目配置
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/project_config.cmake)

# 添加包含目录
include_directories(${INCLUDE_DIRS})

# 定义宏（HAL配置文件中已定义USE_RTOS，此处不重复定义）
add_definitions(-DSTM32F103xB)
add_definitions(-DUSE_HAL_DRIVER)

# 定义编译选项
add_compile_options(
    -Wall
    -Wextra
    $<$<CONFIG:DEBUG>:-O0>
    $<$<CONFIG:RELEASE>:-O2>
    -g
    -fdata-sections
    -ffunction-sections
)

# 创建可执行文件
add_executable(${PROJECT_NAME}.elf
    ${C_SOURCES}
    ${CPP_SOURCES}
    ${ASM_SOURCES}
)

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

# 后处理：生成HEX和BIN文件
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

# 打印配置信息
message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "C Sources: ${C_SOURCES}")
message(STATUS "C++ Sources: ${CPP_SOURCES}")
message(STATUS "ASM Sources: ${ASM_SOURCES}")
message(STATUS "Include Dirs: ${INCLUDE_DIRS}")
