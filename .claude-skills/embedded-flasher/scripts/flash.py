#!/usr/bin/env python3
"""
STM32 固件自动烧录脚本 (Windows/Linux)
配合 embedded-cmake-generator 使用
"""

import os
import sys
import argparse
import subprocess
from pathlib import Path


class Colors:
    """ANSI 颜色代码"""
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    RED = '\033[0;31m'
    NC = '\033[0m'


def log_info(msg):
    print(f"{Colors.GREEN}[INFO]{Colors.NC} {msg}")


def log_warn(msg):
    print(f"{Colors.YELLOW}[WARN]{Colors.NC} {msg}")


def log_error(msg):
    print(f"{Colors.RED}[ERROR]{Colors.NC} {msg}")


def find_openocd():
    """查找 OpenOCD 可执行文件"""
    openocd_names = ['openocd.exe', 'openocd']

    for name in openocd_names:
        try:
            result = subprocess.run(['where', name], capture_output=True, text=True)
            if result.returncode == 0:
                return name
        except FileNotFoundError:
            continue

    return None


def check_build_file(build_dir, project_name='Project', ext='.hex'):
    """检查构建文件是否存在"""
    hex_file = os.path.join(build_dir, f'{project_name}{ext}')

    if not os.path.exists(hex_file):
        log_error(f"未找到构建文件: {hex_file}")
        log_info("请先运行 CMake 生成器并构建项目：")
        log_info("  1. python .claude-skills/embedded-cmake-generator/scripts/generate-cmake.py")
        log_info("  2. mkdir build && cd build")
        log_info("  3. cmake .. -G \"MinGW Makefiles\" -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-toolchain.cmake")
        log_info("  4. make -j4")
        sys.exit(1)

    return hex_file


def flash_firmware(hex_file, debugger, target, speed, mode, openocd_cmd):
    """使用 OpenOCD 烧录固件"""

    commands = [
        '-f', f'interface/{debugger}.cfg',
        '-f', f'target/{target}.cfg',
    ]

    # 添加传输配置和速度
    # 注意：cmsis-dap.cfg 会自动选择 SWD，不需要手动指定
    if debugger.startswith('stlink'):
        commands.extend(['-c', 'transport select hla_adapter_kit'])
        commands.extend(['-c', f'adapter speed {speed}'])
    elif debugger.startswith('jlink'):
        commands.extend(['-c', 'transport select jlink'])
        commands.extend(['-c', f'adapter speed {speed}'])
    elif debugger.startswith('cmsis-dap'):
        # CMSIS-DAP 自动选择 SWD，只设置速度
        commands.extend(['-c', f'adapter speed {speed}'])

    # 添加烧录命令
    # 确保路径使用正斜杠（Windows 兼容）
    hex_file_fixed = hex_file.replace('\\', '/')

    if mode == 'flash':
        commands.extend(['-c', f'program {hex_file_fixed} verify'])
    elif mode == 'verify':
        commands.extend(['-c', f'program {hex_file_fixed} verify'])
    elif mode == 'reset':
        commands.extend(['-c', f'program {hex_file_fixed} verify reset exit'])
    else:
        log_error(f"未知模式: {mode}")
        sys.exit(1)

    log_info(f"开始烧录: {os.path.basename(hex_file)}")
    log_info(f"调试器: {debugger}")
    log_info(f"目标: {target}")
    log_info(f"速度: {speed} kHz")
    log_info(f"模式: {mode}")
    print()

    try:
        result = subprocess.run(
            [openocd_cmd] + commands,
            capture_output=True,
            text=True,
            check=False
        )

        print(result.stdout)
        if result.stderr:
            print(result.stderr, file=sys.stderr)

        return result.returncode == 0
    except FileNotFoundError:
        log_error(f"未找到 OpenOCD: {openocd_cmd}")
        sys.exit(1)
    except Exception as e:
        log_error(f"烧录失败: {e}")
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(
        description='STM32 固件烧录工具',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument(
        '-d', '--debugger',
        default='stlink-v2',
        help='调试器配置 (默认: stlink-v2)'
    )
    parser.add_argument(
        '-t', '--target',
        default='stm32f1x',
        help='目标芯片 (默认: stm32f1x)'
    )
    parser.add_argument(
        '-m', '--mode',
        default='reset',
        choices=['flash', 'verify', 'reset'],
        help='烧录模式: flash(仅烧录), verify(烧录+验证), reset(烧录+验证+复位)'
    )
    parser.add_argument(
        '-s', '--speed',
        default='1000',
        type=int,
        help='烧录速度 kHz (默认: 1000)'
    )
    parser.add_argument(
        '-b', '--build-only',
        action='store_true',
        help='仅构建，不烧录'
    )
    parser.add_argument(
        '--project-dir',
        default='.',
        help='项目目录 (默认: 当前目录)'
    )
    parser.add_argument(
        '--build-dir',
        default='build',
        help='构建目录 (默认: build)'
    )

    args = parser.parse_args()

    log_info("=== STM32 固件烧录工具 ===")

    # 确定项目路径
    project_dir = os.path.abspath(args.project_dir)
    build_dir = os.path.join(project_dir, args.build_dir)

    # 查找 OpenOCD
    openocd_cmd = find_openocd()
    if openocd_cmd:
        log_info(f"找到 OpenOCD: {openocd_cmd}")
    else:
        log_error("未找到 OpenOCD，请安装后重试")
        log_info("下载: https://github.com/openocd-org/openocd/releases")
        sys.exit(1)

    # 检查构建文件
    hex_file = check_build_file(build_dir, ext='.hex')

    # 如果只构建
    if args.build_only:
        log_info("构建完成，跳过烧录")
        log_info(f"固件文件: {hex_file}")
        file_size = os.path.getsize(hex_file)
        log_info(f"  大小: {file_size:,} 字节 ({file_size // 1024} KB)")
        return

    # 烧录
    success = flash_firmware(
        hex_file,
        args.debugger,
        args.target,
        str(args.speed),
        args.mode,
        openocd_cmd
    )

    if success:
        log_info("烧录成功！")
        log_info("固件信息:")
        file_size = os.path.getsize(hex_file)
        file_mtime = os.path.getmtime(hex_file)
        log_info(f"  文件: {os.path.basename(hex_file)}")
        log_info(f"  大小: {file_size:,} 字节 ({file_size // 1024} KB)")
    else:
        log_error("烧录失败！")
        sys.exit(1)


if __name__ == '__main__':
    main()
