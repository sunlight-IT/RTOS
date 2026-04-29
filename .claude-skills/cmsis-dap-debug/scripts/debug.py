#!/usr/bin/env python3
"""
CMSIS-DAP Debug Tool - Main Entry Point

STM32F103C8T6 调试工具，使用 OpenOCD 进行内存、寄存器和执行控制。
"""

import sys
import os
import argparse
import subprocess
from datetime import datetime

# 添加当前目录到 Python 路径
script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
    sys.path.insert(0, script_dir)

from memory import MemoryOperator
from register import RegisterOperator
from breakpoint import BreakpointManager
from execution import ExecutionController


class CMSISDAPDebugger:
    """CMSIS-DAP 调试器主类"""

    # STM32F103C8T6 默认配置
    DEFAULT_INTERFACE = "cmsis-dap"
    DEFAULT_TARGET = "stm32f1x.cfg"
    DEFAULT_TRANSPORT = "swd"
    DEFAULT_SPEED = 1000  # kHz

    # 内存区域定义（基于 STM32F103XX_FLASH.ld）
    MEMORY_REGIONS = {
        "FLASH": {"start": 0x08000000, "size": 0x10000, "desc": "64KB Program Memory"},
        "RAM": {"start": 0x20000000, "size": 0x5000, "desc": "20KB Data Memory"},
        "PERIPHERAL": {"start": 0x40000000, "size": 0x20000000, "desc": "Peripheral Registers"},
    }

    # 外设寄存器映射
    PERIPHERAL_REGISTERS = {
        "GPIOA": {"base": 0x40010800, "regs": ["CRL", "CRH", "IDR", "ODR", "BSRR", "BRR", "LCKR"]},
        "GPIOB": {"base": 0x40010C00, "regs": ["CRL", "CRH", "IDR", "ODR", "BSRR", "BRR", "LCKR"]},
        "GPIOC": {"base": 0x40011000, "regs": ["CRL", "CRH", "IDR", "ODR", "BSRR", "BRR", "LCKR"]},
        "USART1": {"base": 0x40013800, "regs": ["SR", "DR", "BRR", "CR1", "CR2", "CR3", "GTPR"]},
        "USART2": {"base": 0x40004400, "regs": ["SR", "DR", "BRR", "CR1", "CR2", "CR3", "GTPR"]},
    }

    def __init__(self, interface=None, target=None, transport=None, speed=None, output_file=None):
        self.interface = interface or self.DEFAULT_INTERFACE
        self.target = target or self.DEFAULT_TARGET
        self.transport = transport or self.DEFAULT_TRANSPORT
        self.speed = speed or self.DEFAULT_SPEED
        self.output_file = output_file or "debug_output.log"

        # 初始化各个操作模块
        self.memory = MemoryOperator()
        self.register = RegisterOperator()
        self.breakpoint = BreakpointManager()
        self.execution = ExecutionController()

    def build_openocd_commands(self, commands):
        """
        构建 OpenOCD 命令序列

        Args:
            commands: 调试命令列表

        Returns:
            完整的 OpenOCD 命令列表（不含 -c 前缀）
        """
        full_commands = [
            f"interface {self.interface}",
            f"transport select {self.transport}",
            f"target/{self.target}",
            f"adapter speed {self.speed}",
        ]
        full_commands.extend(commands)
        full_commands.append("shutdown")
        return full_commands

    def run_openocd(self, commands):
        """
        执行 OpenOCD 命令

        Args:
            commands: OpenOCD 命令列表

        Returns:
            命令输出
        """
        # 构建命令行 - 参考成功的 flash_test.sh 格式
        cmd = [
            "openocd",
            "-c", f"interface {self.interface}",
            "-c", f"transport select {self.transport}",
            "-f", f"target/{self.target}",
            "-c", f"adapter speed {self.speed}",
        ]

        # 添加初始化和暂停命令
        cmd.extend(["-c", "init"])
        cmd.extend(["-c", "halt"])

        # 添加调试命令
        for c in commands:
            cmd.extend(["-c", c])

        # 添加 shutdown
        cmd.extend(["-c", "shutdown"])

        print(f"[DEBUG] 执行 OpenOCD 命令: {' '.join(cmd[:10])}...")

        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30
            )

            # 保存输出到文件
            self.save_output(result.stdout, result.stderr)

            return result.stdout, result.stderr, result.returncode

        except subprocess.TimeoutExpired:
            error_msg = f"[ERROR] OpenOCD 命令超时（30秒）"
            print(error_msg)
            self.save_output("", error_msg)
            return "", error_msg, -1
        except FileNotFoundError:
            error_msg = "[ERROR] 未找到 OpenOCD，请安装后重试"
            print(error_msg)
            self.save_output("", error_msg)
            return "", error_msg, -1

    def save_output(self, stdout, stderr):
        """
        保存调试输出到文件

        Args:
            stdout: 标准输出
            stderr: 标准错误
        """
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        with open(self.output_file, "a", encoding="utf-8") as f:
            f.write(f"\n{'='*60}\n")
            f.write(f"时间: {timestamp}\n")
            f.write(f"调试器: {self.interface}\n")
            f.write(f"目标: {self.target}\n")
            f.write(f"传输: {self.transport}\n")
            f.write(f"速度: {self.speed} kHz\n")
            f.write(f"{'='*60}\n\n")

            if stdout:
                f.write("[STDOUT]\n")
                f.write(stdout)
                f.write("\n")

            if stderr:
                f.write("[STDERR]\n")
                f.write(stderr)
                f.write("\n")

        print(f"[INFO] 输出已保存到: {self.output_file}")

    # 内存操作接口
    def read_memory(self, addr, count=1, size="w"):
        """
        读取内存

        Args:
            addr: 起始地址（支持 0x 前缀）
            count: 读取数量
            size: 数据大小（w=32bit, h=16bit, b=8bit）
        """
        commands = self.memory.read_memory(addr, count, size)
        stdout, stderr, retcode = self.run_openocd(commands)

        if retcode == 0:
            self.memory.parse_output(stdout, addr, count)
        return retcode == 0

    def write_memory(self, addr, value, size="w"):
        """
        写入内存

        Args:
            addr: 目标地址
            value: 写入值
            size: 数据大小（w=32bit, h=16bit, b=8bit）
        """
        commands = self.memory.write_memory(addr, value, size)
        stdout, stderr, retcode = self.run_openocd(commands)
        return retcode == 0

    # 寄存器操作接口
    def show_registers(self, reg_name=None):
        """
        显示寄存器

        Args:
            reg_name: 寄存器名称（None 显示所有）
        """
        commands = self.register.show_registers(reg_name)
        stdout, stderr, retcode = self.run_openocd(commands)

        if retcode == 0:
            self.register.parse_output(stdout)
        return retcode == 0

    def show_peripheral_registers(self, peripheral):
        """
        显示外设寄存器

        Args:
            peripheral: 外设名称（如 GPIOA, USART1）
        """
        if peripheral not in self.PERIPHERAL_REGISTERS:
            print(f"[ERROR] 未知外设: {peripheral}")
            print(f"[INFO] 可用外设: {', '.join(self.PERIPHERAL_REGISTERS.keys())}")
            return False

        info = self.PERIPHERAL_REGISTERS[peripheral]
        print(f"\n{peripheral} 基地址: 0x{info['base']:08X}")
        print(f"寄存器: {', '.join(info['regs'])}\n")

        for reg in info['regs']:
            reg_addr = info['base'] + info['regs'].index(reg) * 4
            self.read_memory(reg_addr, 1, "w")

        return True

    # 断点管理接口
    def set_breakpoint(self, addr, hardware=False):
        """
        设置断点

        Args:
            addr: 断点地址
            hardware: 是否为硬件断点
        """
        commands = self.breakpoint.set_breakpoint(addr, hardware)
        stdout, stderr, retcode = self.run_openocd(commands)
        return retcode == 0

    def remove_breakpoint(self, bp_num):
        """
        移除断点

        Args:
            bp_num: 断点编号
        """
        commands = self.breakpoint.remove_breakpoint(bp_num)
        stdout, stderr, retcode = self.run_openocd(commands)
        return retcode == 0

    def list_breakpoints(self):
        """列出所有断点"""
        commands = self.breakpoint.list_breakpoints()
        stdout, stderr, retcode = self.run_openocd(commands)

        if retcode == 0:
            self.breakpoint.parse_output(stdout)
        return retcode == 0

    # 执行控制接口
    def halt(self, timeout_ms=0):
        """
        暂停目标

        Args:
            timeout_ms: 超时时间（毫秒）
        """
        commands = self.execution.halt(timeout_ms)
        stdout, stderr, retcode = self.run_openocd(commands)
        return retcode == 0

    def resume(self, addr=None):
        """
        恢复执行

        Args:
            addr: 从指定地址恢复（None 从当前 PC）
        """
        commands = self.execution.resume(addr)
        stdout, stderr, retcode = self.run_openocd(commands)
        return retcode == 0

    def step(self, addr=None):
        """
        单步执行

        Args:
            addr: 从指定地址单步（None 从当前 PC）
        """
        commands = self.execution.step(addr)
        stdout, stderr, retcode = self.run_openocd(commands)
        return retcode == 0

    def reset(self, halt=False):
        """
        复位目标

        Args:
            halt: 复位后是否暂停
        """
        commands = self.execution.reset(halt)
        stdout, stderr, retcode = self.run_openocd(commands)
        return retcode == 0


def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description="CMSIS-DAP 调试工具 - STM32F103C8T6",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  %(prog)s --read-memory 0x08000000 --count 4
  %(prog)s --read-memory 0x20000000 --size h --count 8
  %(prog)s --show-registers
  %(prog)s --show-peripheral GPIOA
  %(prog)s --set-breakpoint 0x08000100
  %(prog)s --halt
  %(prog)s --step
  %(prog)s --reset --halt

内存区域:
  FLASH:  0x08000000 - 0x08010000 (64KB)
  RAM:    0x20000000 - 0x20005000 (20KB)
  外设:    0x40000000 - 0x60000000

外设名称:
  GPIOA, GPIOB, GPIOC, USART1, USART2
        """
    )

    # 通用参数
    parser.add_argument("--interface", default="cmsis-dap",
                      help="调试器接口 (默认: cmsis-dap)")
    parser.add_argument("--target", default="stm32f1x.cfg",
                      help="目标配置文件 (默认: stm32f1x.cfg)")
    parser.add_argument("--transport", default="swd",
                      help="传输协议 (默认: swd)")
    parser.add_argument("--speed", type=int, default=1000,
                      help="调试速度 kHz (默认: 1000)")
    parser.add_argument("--output", "-o", default="debug_output.log",
                      help="输出文件 (默认: debug_output.log)")

    # 内存操作
    memory_group = parser.add_argument_group("内存操作")
    memory_group.add_argument("--read-memory", metavar="ADDR",
                           help="读取内存地址")
    memory_group.add_argument("--count", type=int, default=1,
                           help="读取数量 (默认: 1)")
    memory_group.add_argument("--size", choices=["w", "h", "b"], default="w",
                           help="数据大小: w=32bit, h=16bit, b=8bit (默认: w)")

    # 寄存器操作
    register_group = parser.add_argument_group("寄存器操作")
    register_group.add_argument("--show-registers", dest="show_registers",
                            action="append_const", const=None,
                            help="显示寄存器 (可选: 指定寄存器名称)")
    register_group.add_argument("--show-peripheral", metavar="PERIPH",
                            help="显示外设寄存器 (GPIOA, GPIOB, GPIOC, USART1, USART2)")

    # 断点管理
    bp_group = parser.add_argument_group("断点管理")
    bp_group.add_argument("--set-breakpoint", metavar="ADDR",
                        help="设置断点")
    bp_group.add_argument("--remove-breakpoint", metavar="NUM",
                        type=int, help="移除断点编号")
    bp_group.add_argument("--list-breakpoints", action="store_true",
                        help="列出所有断点")
    bp_group.add_argument("--hardware-bp", action="store_true",
                        help="使用硬件断点")

    # 执行控制
    exec_group = parser.add_argument_group("执行控制")
    exec_group.add_argument("--halt", action="store_true",
                         help="暂停目标")
    exec_group.add_argument("--resume", action="store_true",
                         help="恢复执行")
    exec_group.add_argument("--step", action="store_true",
                         help="单步执行")
    exec_group.add_argument("--reset", action="store_true",
                         help="复位目标")

    args = parser.parse_args()

    # 创建调试器实例
    debugger = CMSISDAPDebugger(
        interface=args.interface,
        target=args.target,
        transport=args.transport,
        speed=args.speed,
        output_file=args.output
    )

    # 执行相应的操作
    success = True

    # 内存操作
    if args.read_memory:
        addr = args.read_memory
        if not addr.startswith("0x"):
            addr = f"0x{addr}"
        success = debugger.read_memory(addr, args.count, args.size)

    elif args.show_registers is not None:
        # 处理 append_const 的情况
        reg_value = args.show_registers
        if isinstance(reg_value, list) and len(reg_value) > 0:
            reg_value = reg_value[0]
        success = debugger.show_registers(reg_value)

    elif args.show_peripheral:
        success = debugger.show_peripheral_registers(args.show_peripheral.upper())

    # 断点管理
    elif args.set_breakpoint:
        addr = args.set_breakpoint
        if not addr.startswith("0x"):
            addr = f"0x{addr}"
        success = debugger.set_breakpoint(addr, args.hardware_bp)

    elif args.remove_breakpoint is not None:
        success = debugger.remove_breakpoint(args.remove_breakpoint)

    elif args.list_breakpoints:
        success = debugger.list_breakpoints()

    # 执行控制
    elif args.halt:
        success = debugger.halt()

    elif args.resume:
        success = debugger.resume()

    elif args.step:
        success = debugger.step()

    elif args.reset:
        success = debugger.reset(args.hardware_bp)

    else:
        parser.print_help()

    # 返回状态码
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
