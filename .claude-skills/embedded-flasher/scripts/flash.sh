#!/bin/bash
#
# STM32 固件自动烧录脚本
# 配合 embedded-cmake-generator 使用
#

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 配置
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
HEX_FILE="${BUILD_DIR}/${PROJECT_NAME:-Project}.hex"
OPENOCD_CMD="openocd"

# 默认参数
DEBUGGER="stlink-v2"
TARGET="stm32f1x"
MODE="reset"
SPEED="1000"
BUILD_ONLY=false
VERIFY=true

# 打印信息
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 显示帮助
show_help() {
    cat << EOF
STM32 固件烧录脚本

用法: $0 [选项]

选项:
    -d, --debugger DBG   调试器配置 (默认: stlink-v2)
                          支持: stlink-v2, jlink, cmsis-dap
    -t, --target TGT      目标芯片 (默认: stm32f1x)
    -m, --mode MODE       烧录模式:
                          flash   - 仅烧录
                          verify  - 烧录+验证
                          reset   - 烧录+验证+复位 (默认)
    -s, --speed SPD      烧录速度 kHz (默认: 1000)
    -b, --build-only      仅构建，不烧录
    -h, --help           显示此帮助信息

示例:
    $0                           # 默认模式：烧录+验证+复位
    $0 --mode flash            # 仅烧录
    $0 --debugger cmsis-dap   # 使用 CMSIS-DAP
    $0 --build-only             # 只构建不烧录

EOF
}

# 解析参数
while [[ $# -gt 0 ]]; do
    case "$1" in
        -d|--debugger)
            DEBUGGER="$2"
            shift 2
            ;;
        --debugger=*)  # 支持长格式 --debugger=VALUE
            DEBUGGER="${1#*=}"
            shift
            ;;
        -t|--target)
            TARGET="$2"
            shift
            ;;
        -m|--mode)
            MODE="$2"
            shift
            ;;
        -s|--speed)
            SPEED="$2"
            shift
            ;;
        -b|--build-only)
            BUILD_ONLY=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            log_error "未知选项: $1"
            show_help
            exit 1
            ;;
    esac
done

# 检查 OpenOCD
check_openocd() {
    if ! command -v ${OPENOCD_CMD} &> /dev/null; then
        log_error "未找到 OpenOCD，请安装: sudo apt install openocd"
        exit 1
    fi
}

# 检查构建文件
check_build_file() {
    local ext="${1:-.hex}"
    local file="${BUILD_DIR}/${PROJECT_NAME:-Project}${ext}"

    if [[ ! -f "$file" ]]; then
        log_error "未找到构建文件: $file"
        log_info "请先运行 CMake 生成器并构建项目："
        log_info "  1. python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py"
        log_info "  2. mkdir build && cd build"
        log_info "  3. cmake .. -G \"MinGW Makefiles\" -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-toolchain.cmake"
        log_info "  4. make -j4"
        exit 1
    fi
    echo "$file"
}

# 烧录固件
flash_firmware() {
    local hex_file="$1"
    local commands=(
        "-f interface/${DEBUGGER}.cfg"
        "-f target/${TARGET}.cfg"
    )

    # 添加传输配置和速度
    case $DEBUGGER in
        stlink-v2*)
            commands+=("-c \"transport select hla_adapter_kit\"")
            commands+=("-c \"adapter speed ${SPEED}\"")
            ;;
        jlink*)
            commands+=("-c \"transport select jlink\"")
            commands+=("-c \"adapter speed ${SPEED}\"")
            ;;
        cmsis-dap*)
            # CMSIS-DAP 自动选择 SWD，只设置速度
            commands+=("-c \"adapter speed ${SPEED}\"")
            ;;
        *)
            commands+=("-c \"adapter speed ${SPEED}\"")
            ;;
    esac

    case $MODE in
        flash)
            commands+=("-c \"program ${hex_file} verify\"")
            ;;
        verify)
            commands+=("-c \"program ${hex_file} verify\"")
            ;;
        reset)
            commands+=("-c \"program ${hex_file} verify reset exit\"")
            ;;
        *)
            log_error "未知模式: $MODE"
            exit 1
            ;;
    esac

    log_info "开始烧录: $(basename "$hex_file")"
    log_info "调试器: $DEBUGGER"
    log_info "目标: $TARGET"
    log_info "速度: ${SPEED} kHz"
    log_info "模式: $MODE"

    ${OPENOCD_CMD} "${commands[@]}" 2>&1
}

# 主流程
main() {
    log_info "=== STM32 固件烧录工具 ==="

    # 检查依赖
    check_openocd

    # 检查构建文件
    hex_file=$(check_build_file ".hex")

    # 如果只构建，跳过烧录
    if [[ "$BUILD_ONLY" == true ]]; then
        log_info "构建完成，跳过烧录"
        log_info "固件文件: $hex_file"
        log_info "$(ls -lh "$hex_file" | awk '{print $9, $5}')"
        exit 0
    fi

    # 烧录
    flash_firmware "$hex_file"

    # 检查结果
    if [[ $? -eq 0 ]]; then
        log_info "烧录成功！"
        log_info "固件信息:"
        ls -lh "$hex_file" | awk '{print "  大小: " $9, " 时间: " $6, $7, $8}'
    else
        log_error "烧录失败！"
        exit 1
    fi
}

# 运行主流程
main
