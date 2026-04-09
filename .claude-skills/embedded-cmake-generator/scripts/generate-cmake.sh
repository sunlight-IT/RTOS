#!/bin/bash
######################################################################
# STM32 CMake Project Generator
# 自动扫描项目目录并生成CMakeLists.txt
######################################################################

set -e

# 默认配置
PROJECT_DIR="$(pwd)"
OUTPUT_DIR="cmake"
PROJECT_NAME=""

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# 帮助信息
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

生成STM32项目的CMake构建配置。

OPTIONS:
    -d, --project-dir DIR     项目根目录 (默认: 当前目录)
    -o, --output-dir DIR     CMake文件输出目录 (默认: cmake/)
    -n, --project-name NAME  项目名称 (默认: 自动检测)
    -h, --help               显示此帮助信息

EXAMPLES:
    $0
    $0 --project-dir /path/to/project
    $0 -n my_project

EOF
    exit 0
}

# 解析参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--project-dir)
            PROJECT_DIR="$2"
            shift 2
            ;;
        -o|--output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        -n|--project-name)
            PROJECT_NAME="$2"
            shift 2
            ;;
        -h|--help)
            usage
            ;;
        *)
            log_error "未知选项: $1"
            usage
            ;;
    esac
done

# 检查项目目录是否存在
if [ ! -d "$PROJECT_DIR" ]; then
    log_error "项目目录不存在: $PROJECT_DIR"
    exit 1
fi

log_info "项目目录: $PROJECT_DIR"
log_info "输出目录: $OUTPUT_DIR"

# 自动检测项目名称
if [ -z "$PROJECT_NAME" ]; then
    if [ -f "$PROJECT_DIR/Makefile" ]; then
        PROJECT_NAME=$(grep "^TARGET = " "$PROJECT_DIR/Makefile" | cut -d' ' -f3)
    fi
    if [ -z "$PROJECT_NAME" ] && [ -f "$PROJECT_DIR/Project.ioc" ]; then
        PROJECT_NAME="Project"
    fi
    if [ -z "$PROJECT_NAME" ]; then
        PROJECT_NAME=$(basename "$PROJECT_DIR")
    fi
fi
log_info "项目名称: $PROJECT_NAME"

# 创建输出目录
mkdir -p "$PROJECT_DIR/$OUTPUT_DIR"

log_info "开始扫描项目文件..."

# 排除的目录
EXCLUDE_DIRS="build cmake-build Debug Release .git .vscode MDK-ARM EWARM .settings"

# 扫描C源文件
log_info "扫描C源文件..."
C_SOURCES=$(find "$PROJECT_DIR" -type f \( -name "*.c" \) \
    $(echo "$EXCLUDE_DIRS" | sed 's/\([^ ]*\)/-path "*\1*" -prune -o/g') \
    -print | grep -v ".git" | sort)

# 扫描C++源文件
log_info "扫描C++源文件..."
CPP_SOURCES=$(find "$PROJECT_DIR" -type f \( -name "*.cpp" -o -name "*.cxx" -o -name "*.cc" \) \
    $(echo "$EXCLUDE_DIRS" | sed 's/\([^ ]*\)/-path "*\1*" -prune -o/g') \
    -print | grep -v ".git" | sort)

# 扫描汇编文件
log_info "扫描汇编文件..."
ASM_SOURCES=$(find "$PROJECT_DIR" -type f \( -name "*.s" -o -name "*.S" \) \
    $(echo "$EXCLUDE_DIRS" | sed 's/\([^ ]*\)/-path "*\1*" -prune -o/g') \
    -print | grep -v ".git" | sort)

# 提取头文件包含目录
log_info "扫描头文件目录..."
INCLUDE_DIRS=$(find "$PROJECT_DIR" -type f \( -name "*.h" -o -name "*.hpp" \) \
    $(echo "$EXCLUDE_DIRS" | sed 's/\([^ ]*\)/-path "*\1*" -prune -o/g') \
    -print | grep -v ".git" | xargs dirname | sort -u)

# 查找启动文件
STARTUP_FILE=$(find "$PROJECT_DIR" -maxdepth 1 -name "startup_stm32*.s" -o -name "startup_stm32*.S" | head -1)
if [ -z "$STARTUP_FILE" ]; then
    log_warn "未找到启动文件"
else
    log_info "启动文件: $(basename "$STARTUP_FILE")"
fi

# 查找链接脚本
LINKER_SCRIPT=$(find "$PROJECT_DIR" -maxdepth 1 -name "*FLASH.ld" | head -1)
if [ -z "$LINKER_SCRIPT" ]; then
    log_warn "未找到链接脚本"
else
    log_info "链接脚本: $(basename "$LINKER_SCRIPT")"
fi

# 统计文件数量
C_COUNT=$(echo "$C_SOURCES" | grep -c . || echo 0)
CPP_COUNT=$(echo "$CPP_SOURCES" | grep -c . || echo 0)
ASM_COUNT=$(echo "$ASM_SOURCES" | grep -c . || echo 0)
INC_COUNT=$(echo "$INCLUDE_DIRS" | grep -c . || echo 0)

log_info "找到: $C_COUNT 个C文件, $CPP_COUNT 个C++文件, $ASM_COUNT 个汇编文件, $INC_COUNT 个头文件目录"

# 生成项目配置文件
log_info "生成项目配置文件..."

# 计算项目目录路径长度以便用cut截取
PROJECT_PREFIX="${PROJECT_DIR}/"

cat > "$PROJECT_DIR/$OUTPUT_DIR/project_config.cmake" << EOF
# STM32 项目配置
# 自动生成于: $(date)

set(PROJECT_NAME "${PROJECT_NAME}")

# C 源文件
set(C_SOURCES
$(echo "$C_SOURCES" | while read -r file; do echo "    \${CMAKE_CURRENT_SOURCE_DIR}/$(echo "$file" | sed "s|^$PROJECT_PREFIX||")"; done)
)

# C++ 源文件
set(CPP_SOURCES
$(echo "$CPP_SOURCES" | while read -r file; do echo "    \${CMAKE_CURRENT_SOURCE_DIR}/$(echo "$file" | sed "s|^$PROJECT_PREFIX||")"; done)
)

# 汇编源文件
set(ASM_SOURCES
$(echo "$ASM_SOURCES" | while read -r file; do echo "    \${CMAKE_CURRENT_SOURCE_DIR}/$(echo "$file" | sed "s|^$PROJECT_PREFIX||")"; done)
)

# 头文件包含目录
set(INCLUDE_DIRS
$(echo "$INCLUDE_DIRS" | while read -r dir; do echo "    \${CMAKE_CURRENT_SOURCE_DIR}/$(echo "$dir" | sed "s|^$PROJECT_PREFIX||")"; done)
)

# 链接脚本
EOF

if [ -n "$LINKER_SCRIPT" ]; then
    echo "set(LINKER_SCRIPT \"\${CMAKE_CURRENT_SOURCE_DIR}/$(basename "$LINKER_SCRIPT")\")" >> "$PROJECT_DIR/$OUTPUT_DIR/project_config.cmake"
else
    echo "# 未找到链接脚本" >> "$PROJECT_DIR/$OUTPUT_DIR/project_config.cmake"
fi

# 生成工具链配置文件
log_info "生成工具链配置文件..."
cat > "$PROJECT_DIR/$OUTPUT_DIR/arm-none-eabi-toolchain.cmake" << 'EOF'
# ARM GCC 工具链配置

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
set(CPU_FLAGS
    -mcpu=cortex-m3
    -mthumb
    -mfloat-abi=soft
)

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
EOF

# 生成主CMakeLists.txt
log_info "生成主CMakeLists.txt..."
cat > "$PROJECT_DIR/CMakeLists.txt" << EOF
cmake_minimum_required(VERSION 3.20)
project(${PROJECT_NAME} C CXX ASM)

# 工具链配置
set(CMAKE_TOOLCHAIN_FILE \${CMAKE_CURRENT_SOURCE_DIR}/${OUTPUT_DIR}/arm-none-eabi-toolchain.cmake CACHE STRING "" FORCE)

# 加载项目配置
include(\${CMAKE_CURRENT_SOURCE_DIR}/${OUTPUT_DIR}/project_config.cmake)

# 添加包含目录
include_directories(\${INCLUDE_DIRS})

# 定义编译选项
add_compile_options(
    -Wall
    -Wextra
    \$<\$<CONFIG:DEBUG>:-O0>
    \$<\$<CONFIG:RELEASE>:-O2>
    -g
    -fdata-sections
    -ffunction-sections
)

# 创建可执行文件
add_executable(\${PROJECT_NAME}.elf
    \${C_SOURCES}
    \${CPP_SOURCES}
    \${ASM_SOURCES}
)

# 设置链接脚本
if(DEFINED LINKER_SCRIPT)
    target_link_options(\${PROJECT_NAME}.elf PRIVATE
        -T\${LINKER_SCRIPT}
    )
endif()

# 链接库
target_link_libraries(\${PROJECT_NAME}.elf
    c
    m
    nosys
)

# 后处理：生成HEX和BIN文件
add_custom_command(TARGET \${PROJECT_NAME}.elf POST_BUILD
    COMMAND \${CMAKE_OBJCOPY} -O ihex \$<TARGET_FILE:\${PROJECT_NAME}.elf> \${PROJECT_NAME}.hex
    COMMAND \${CMAKE_OBJCOPY} -O binary \$<TARGET_FILE:\${PROJECT_NAME}.elf> \${PROJECT_NAME}.bin
    COMMAND \${CMAKE_SIZE} \$<TARGET_FILE:\${PROJECT_NAME}.elf>
    COMMENT "Generating .hex and .bin files"
)

# 设置输出目录
set_target_properties(\${PROJECT_NAME}.elf PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY \${CMAKE_BINARY_DIR}
    OUTPUT_NAME \${PROJECT_NAME}
)

# 打印配置信息
message(STATUS "Project Name: \${PROJECT_NAME}")
message(STATUS "C Sources: \${C_SOURCES}")
message(STATUS "C++ Sources: \${CPP_SOURCES}")
message(STATUS "ASM Sources: \${ASM_SOURCES}")
message(STATUS "Include Dirs: \${INCLUDE_DIRS}")
EOF

log_info "CMake工程生成完成！"
echo ""
log_info "下一步："
echo "  1. 创建构建目录: mkdir -p build && cd build"
echo "  2. 配置CMake: cmake .."
echo "  3. 编译项目: make -j\$(nproc)"
