#!/usr/bin/env python3
"""
STM32 CMake Project Generator
自动扫描项目目录并生成CMakeLists.txt
"""

import os
import sys
import argparse
from pathlib import Path
from datetime import datetime


def log_info(msg):
    print(f"\033[0;32m[INFO]\033[0m {msg}")


def log_warn(msg):
    print(f"\033[1;33m[WARN]\033[0m {msg}")


def log_error(msg):
    print(f"\033[0;31m[ERROR]\033[0m {msg}")


EXCLUDE_DIRS = {
    'build', 'cmake-build', 'Debug', 'Release',
    '.git', '.vscode', 'MDK-ARM', 'EWARM', '.settings',
    '__pycache__', '.idea', 'Template', 'DSP_Lib_TestSuite',
    'Examples', 'Core_A', 'DSP', 'NN', 'RTOS2', 'Templates',
    'RVDS'  # Exclude ARM RealView compiler specific files
}


def should_exclude(path):
    """检查是否应该排除该目录"""
    for part in Path(path).parts:
        # 排除特定目录
        if part in EXCLUDE_DIRS:
            return True
        # 排除所有 build 开头的目录
        if part.startswith('build'):
            return True
    return False


def scan_sources(project_dir):
    """扫描源文件"""
    c_sources = []
    cpp_sources = []
    asm_sources = []

    # 排除的文件列表
    excluded_files = {
        # FreeRTOS 只保留 heap_4.c，排除其他 heap 实现
        'heap_1.c', 'heap_2.c', 'heap_3.c', 'heap_5.c',
        # 只使用 CMSIS-RTOS V2，排除 V1
        'cmsis_os.c', 'cmsis_os1.c',
        # 排除 syscalls.c (标准库系统调用，裸机环境不需要)
        'syscalls.c',
        # 排除 SEGGER RTT 汇编优化文件 (C 编译器处理问题)
        'SEGGER_RTT_ASM_ARMv7M.S',
    }

    for root, dirs, files in os.walk(project_dir):
        # 排除目录
        if should_exclude(root):
            continue

        # 修改 dirs 原地以跳过子目录
        dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]

        for file in files:
            filepath = os.path.join(root, file)

            # 排除模板文件
            if 'template' in file.lower():
                continue

            # 排除特定文件
            if file in excluded_files:
                continue

            if file.endswith('.c'):
                c_sources.append(filepath)
            elif file.endswith(('.cpp', '.cxx', '.cc')):
                cpp_sources.append(filepath)
            elif file.endswith(('.s', '.S')):
                asm_sources.append(filepath)

    return sorted(c_sources), sorted(cpp_sources), sorted(asm_sources)


def scan_headers(project_dir):
    """扫描头文件目录"""
    header_dirs = set()

    for root, dirs, files in os.walk(project_dir):
        # 排除目录
        if should_exclude(root):
            continue

        # 修改 dirs 原地以跳过子目录
        dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]

        # 跳过 CMSIS_RTOS V1 目录（只使用 V2）
        dirs[:] = [d for d in dirs if d != 'CMSIS_RTOS']

        for file in files:
            if file.endswith(('.h', '.hpp')):
                header_dirs.add(root)

    return sorted(header_dirs)


def find_startup_file(project_dir):
    """查找启动文件"""
    for file in os.listdir(project_dir):
        if file.startswith('startup_stm32') and file.endswith(('.s', '.S')):
            return os.path.join(project_dir, file)
    return None


def find_linker_script(project_dir):
    """查找链接脚本"""
    for file in os.listdir(project_dir):
        if 'FLASH' in file and file.endswith('.ld'):
            return os.path.join(project_dir, file)
    return None


def detect_project_name(project_dir):
    """检测项目名称"""
    makefile_path = os.path.join(project_dir, 'Makefile')
    if os.path.exists(makefile_path):
        try:
            with open(makefile_path, 'r', encoding='utf-8', errors='ignore') as f:
                for line in f:
                    if line.strip().startswith('TARGET ='):
                        return line.split('=')[1].strip()
        except:
            pass

    ioc_path = os.path.join(project_dir, 'Project.ioc')
    if os.path.exists(ioc_path):
        return 'Project'

    return os.path.basename(project_dir)


def generate_cmake_files(project_dir, output_dir, project_name):
    """生成CMake文件"""
    output_path = os.path.join(project_dir, output_dir)
    os.makedirs(output_path, exist_ok=True)

    log_info("扫描C源文件...")
    c_sources, cpp_sources, asm_sources = scan_sources(project_dir)

    log_info("扫描头文件目录...")
    header_dirs = scan_headers(project_dir)

    startup_file = find_startup_file(project_dir)
    if startup_file:
        log_info(f"启动文件: {os.path.basename(startup_file)}")
    else:
        log_warn("未找到启动文件")

    linker_script = find_linker_script(project_dir)
    if linker_script:
        log_info(f"链接脚本: {os.path.basename(linker_script)}")
    else:
        log_warn("未找到链接脚本")

    log_info(f"找到: {len(c_sources)} 个C文件, {len(cpp_sources)} 个C++文件, "
              f"{len(asm_sources)} 个汇编文件, {len(header_dirs)} 个头文件目录")

    # 生成项目配置文件
    log_info("生成项目配置文件...")
    project_config_path = os.path.join(output_path, 'project_config.cmake')
    with open(project_config_path, 'w', encoding='utf-8') as f:
        f.write(f"# STM32 项目配置\n")
        f.write(f"# 自动生成于: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")

        f.write(f'set(PROJECT_NAME "{project_name}")\n\n')

        # C源文件
        f.write("# C 源文件\n")
        f.write("set(C_SOURCES\n")
        for src in c_sources:
            rel_path = os.path.relpath(src, project_dir).replace('\\', '/')
            f.write(f'    ${{CMAKE_CURRENT_SOURCE_DIR}}/{rel_path}\n')
        f.write(")\n\n")

        # C++源文件
        f.write("# C++ 源文件\n")
        f.write("set(CPP_SOURCES\n")
        for src in cpp_sources:
            rel_path = os.path.relpath(src, project_dir).replace('\\', '/')
            f.write(f'    ${{CMAKE_CURRENT_SOURCE_DIR}}/{rel_path}\n')
        f.write(")\n\n")

        # 汇编源文件
        f.write("# 汇编源文件\n")
        f.write("set(ASM_SOURCES\n")
        for src in asm_sources:
            rel_path = os.path.relpath(src, project_dir).replace('\\', '/')
            f.write(f'    ${{CMAKE_CURRENT_SOURCE_DIR}}/{rel_path}\n')
        f.write(")\n\n")

        # 头文件包含目录
        f.write("# 头文件包含目录\n")
        f.write("set(INCLUDE_DIRS\n")
        for dir in header_dirs:
            rel_path = os.path.relpath(dir, project_dir).replace('\\', '/')
            f.write(f'    ${{CMAKE_CURRENT_SOURCE_DIR}}/{rel_path}\n')
        f.write(")\n\n")

        # 链接脚本
        if linker_script:
            f.write(f'set(LINKER_SCRIPT "${{CMAKE_CURRENT_SOURCE_DIR}}/{os.path.basename(linker_script)}")\n')
        else:
            f.write("# 未找到链接脚本\n")

    # 生成工具链配置文件
    log_info("生成工具链配置文件...")
    toolchain_path = os.path.join(output_path, 'arm-none-eabi-toolchain.cmake')
    with open(toolchain_path, 'w', encoding='utf-8') as f:
        f.write("""# ARM GCC 工具链配置

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(TOOLCHAIN_PREFIX arm-none-eabi-)

find_program(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
find_program(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
find_program(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
find_program(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
find_program(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}objdump)
find_program(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size)

set(CMAKE_EXECUTABLE_SUFFIX_ASM .elf)
set(CMAKE_EXECUTABLE_SUFFIX_C .elf)
set(CMAKE_EXECUTABLE_SUFFIX_CXX .elf)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# CPU 配置 (STM32F103C8T6 - Cortex-M3)
set(CPU_FLAGS "-mcpu=cortex-m3 -mthumb -mfloat-abi=soft")

set(CMAKE_C_FLAGS "${CPU_FLAGS} -Wall -Wextra -g -fdata-sections -ffunction-sections")
set(CMAKE_CXX_FLAGS "${CPU_FLAGS} -Wall -Wextra -g -fdata-sections -ffunction-sections")
set(CMAKE_ASM_FLAGS "${CPU_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS "${CPU_FLAGS} -specs=nano.specs -Wl,--gc-sections")

# 调试模式添加 -O0
set(CMAKE_C_FLAGS_DEBUG "-O0 ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 ${CMAKE_CXX_FLAGS}")

# 发布模式添加 -O2
set(CMAKE_C_FLAGS_RELEASE "-O2 ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 ${CMAKE_CXX_FLAGS}")
""")

    # 生成主CMakeLists.txt
    log_info("生成主CMakeLists.txt...")
    cmake_path = os.path.join(project_dir, 'CMakeLists.txt')
    with open(cmake_path, 'w', encoding='utf-8') as f:
        f.write(f"""cmake_minimum_required(VERSION 3.20)
project({project_name} C CXX ASM)

# 工具链配置
set(CMAKE_TOOLCHAIN_FILE ${{CMAKE_CURRENT_SOURCE_DIR}}/{output_dir}/arm-none-eabi-toolchain.cmake CACHE STRING "" FORCE)

# 加载项目配置
include(${{CMAKE_CURRENT_SOURCE_DIR}}/{output_dir}/project_config.cmake)

# 添加包含目录
include_directories(${{INCLUDE_DIRS}})

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
add_executable(${{PROJECT_NAME}}.elf
    ${{C_SOURCES}}
    ${{CPP_SOURCES}}
    ${{ASM_SOURCES}}
)

# 设置链接脚本
if(DEFINED LINKER_SCRIPT)
    target_link_options(${{PROJECT_NAME}}.elf PRIVATE
        -T${{LINKER_SCRIPT}}
    )
endif()

# 链接库
target_link_libraries(${{PROJECT_NAME}}.elf
    c
    m
    nosys
)

# 后处理：生成HEX和BIN文件
add_custom_command(TARGET ${{PROJECT_NAME}}.elf POST_BUILD
    COMMAND ${{CMAKE_OBJCOPY}} -O ihex $<TARGET_FILE:${{PROJECT_NAME}}.elf> ${{CMAKE_BINARY_DIR}}/${{PROJECT_NAME}}.hex
    COMMAND ${{CMAKE_OBJCOPY}} -O binary $<TARGET_FILE:${{PROJECT_NAME}}.elf> ${{CMAKE_BINARY_DIR}}/${{PROJECT_NAME}}.bin
    COMMAND ${{CMAKE_SIZE}} $<TARGET_FILE:${{PROJECT_NAME}}.elf>
    COMMENT "Generating .hex and .bin files"
)

# 设置输出目录
set_target_properties(${{PROJECT_NAME}}.elf PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${{CMAKE_BINARY_DIR}}
    OUTPUT_NAME ${{PROJECT_NAME}}
)

# 打印配置信息
message(STATUS "Project Name: ${{PROJECT_NAME}}")
message(STATUS "C Sources: ${{C_SOURCES}}")
message(STATUS "C++ Sources: ${{CPP_SOURCES}}")
message(STATUS "ASM Sources: ${{ASM_SOURCES}}")
message(STATUS "Include Dirs: ${{INCLUDE_DIRS}}")
""")

    log_info("CMake工程生成完成！")
    print()
    log_info("自动排除：")
    print("  - RVDS/ 目录（ARM RealView 编译器专用）")
    print("  - CMSIS_RTOS V1（使用 V2 API）")
    print("  - heap_1/2/3/5.c（使用 heap_4.c）")
    print("  - build* 目录（构建临时文件）")
    print()
    log_info("下一步：")
    print("  1. 创建构建目录: mkdir -p build && cd build")
    print(f"  2. 配置CMake: cmake .. -G \"MinGW Makefiles\" -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-toolchain.cmake")
    print("  3. 编译项目: make -j4")


def main():
    parser = argparse.ArgumentParser(description='生成STM32项目的CMake构建配置')
    parser.add_argument('-d', '--project-dir', default='.', help='项目根目录 (默认: 当前目录)')
    parser.add_argument('-o', '--output-dir', default='cmake', help='CMake文件输出目录 (默认: cmake/)')
    parser.add_argument('-n', '--project-name', default='', help='项目名称 (默认: 自动检测)')

    args = parser.parse_args()

    project_dir = os.path.abspath(args.project_dir)

    if not os.path.isdir(project_dir):
        log_error(f"项目目录不存在: {project_dir}")
        sys.exit(1)

    log_info(f"项目目录: {project_dir}")
    log_info(f"输出目录: {args.output_dir}")

    project_name = args.project_name or detect_project_name(project_dir)
    log_info(f"项目名称: {project_name}")

    generate_cmake_files(project_dir, args.output_dir, project_name)


if __name__ == '__main__':
    main()
