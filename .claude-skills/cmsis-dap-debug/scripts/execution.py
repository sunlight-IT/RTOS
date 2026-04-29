"""
Execution Controller Module

处理 STM32F103C8T6 的执行控制操作。
使用 OpenOCD 的 halt, resume, step, reset 命令。
"""


class ExecutionController:
    """执行控制类"""

    def __init__(self):
        self.current_state = "unknown"  # unknown, halted, running

    def halt(self, timeout_ms=0):
        """
        生成暂停目标的 OpenOCD 命令

        Args:
            timeout_ms: 超时时间（毫秒），0 表示立即暂停

        Returns:
            OpenOCD 命令列表
        """
        if timeout_ms > 0:
            return [f"halt {timeout_ms}"]
        else:
            return ["halt"]

    def resume(self, addr=None):
        """
        生成恢复执行的 OpenOCD 命令

        Args:
            addr: 从指定地址恢复（None 从当前 PC）

        Returns:
            OpenOCD 命令列表
        """
        if addr is not None:
            # 确保地址格式正确
            if isinstance(addr, int):
                addr_hex = f"0x{addr:X}"
            elif isinstance(addr, str):
                if not addr.startswith("0x"):
                    addr_hex = f"0x{int(addr, 0):X}"
                else:
                    addr_hex = addr
            else:
                raise ValueError(f"无效的地址格式: {addr}")
            return [f"resume {addr_hex}"]
        else:
            return ["resume"]

    def step(self, addr=None):
        """
        生成单步执行的 OpenOCD 命令

        Args:
            addr: 从指定地址单步（None 从当前 PC）

        Returns:
            OpenOCD 命令列表
        """
        if addr is not None:
            # 确保地址格式正确
            if isinstance(addr, int):
                addr_hex = f"0x{addr:X}"
            elif isinstance(addr, str):
                if not addr.startswith("0x"):
                    addr_hex = f"0x{int(addr, 0):X}"
                else:
                    addr_hex = addr
            else:
                raise ValueError(f"无效的地址格式: {addr}")
            return [f"step {addr_hex}"]
        else:
            return ["step"]

    def reset(self, halt=False):
        """
        生成复位目标的 OpenOCD 命令

        Args:
            halt: 复位后是否暂停

        Returns:
            OpenOCD 命令列表
        """
        if halt:
            return ["reset halt"]
        else:
            return ["reset"]

    def init_reset_halt(self):
        """
        生成初始化、复位、暂停的完整命令序列

        Returns:
            OpenOCD 命令列表
        """
        return ["init", "reset", "halt"]

    def wait_halt(self, timeout_ms=5000):
        """
        生成等待暂停的 OpenOCD 命令

        Args:
            timeout_ms: 超时时间（毫秒）

        Returns:
            OpenOCD 命令列表
        """
        return [f"wait_halt {timeout_ms}"]

    def verify_halt(self):
        """
        生成验证暂停状态的 OpenOCD 命令

        Returns:
            OpenOCD 命令列表
        """
        return ["capture halt"]

    def get_target_state(self):
        """
        生成获取目标状态的 OpenOCD 命令

        Returns:
            OpenOCD 命令列表
        """
        return ["capture print"]

    def parse_state_output(self, output):
        """
        解析目标状态输出

        Args:
            output: OpenOCD 标准输出
        """
        print(f"\n{'='*60}")
        print("目标状态")
        print(f"{'='*60}\n")

        lines = output.strip().split("\n")

        for line in lines:
            line = line.strip()

            # 检查暂停状态
            if "target state: halted" in line.lower():
                print("目标状态: 已暂停 (HALTED)")
                self.current_state = "halted"

            elif "target state: running" in line.lower():
                print("目标状态: 运行中 (RUNNING)")
                self.current_state = "running"

            elif "target state: reset" in line.lower():
                print("目标状态: 复位中 (RESET)")
                self.current_state = "reset"

            # 显示其他重要信息
            elif "pc" in line.lower() and ":" in line:
                print(line)

            elif "halted" in line.lower():
                print(line)

    def parse_step_output(self, output):
        """
        解析单步执行输出

        Args:
            output: OpenOCD 标准输出
        """
        print(f"\n{'='*60}")
        print("单步执行结果")
        print(f"{'='*60}\n")

        lines = output.strip().split("\n")

        for line in lines:
            line = line.strip()
            print(line)

            # 提取新的 PC 值
            if "pc :" in line.lower():
                parts = line.split(":")
                if len(parts) >= 2:
                    pc_value = parts[1].strip()
                    print(f"\n[INFO] 程序计数器 (PC): {pc_value}")

    def get_stack_trace(self, max_depth=10):
        """
        生成获取栈回溯的 OpenOCD 命令

        注意：栈回溯通常需要通过 GDB 设置

        Args:
            max_depth: 最大回溯深度

        Returns:
            OpenOCD 命令列表
        """
        return [
            "# 栈回溯需要通过 GDB 设置",
            "# 使用 GDB 命令: bt 或 backtrace",
            "# 或者: bt full 显示局部变量"
        ]

    def get_call_stack(self):
        """
        生成获取调用栈的 OpenOCD 命令

        Returns:
            OpenOCD 命令列表
        """
        return [
            "# 调用栈信息通过 GDB 设置",
            "# 使用 GDB 命令: where"
        ]

    def print_execution_info(self):
        """打印执行控制信息"""
        print(f"\nSTM32F103C8T6 执行控制:")
        print("=" * 60)
        print("halt      - 暂停目标")
        print("resume    - 恢复执行")
        print("step      - 单步执行")
        print("reset     - 复位目标")
        print("reset halt - 复位并暂停")
        print("=" * 60)

    def print_current_state(self):
        """打印当前目标状态"""
        state_desc = {
            "unknown": "未知",
            "halted": "已暂停",
            "running": "运行中",
            "reset": "复位中",
        }

        print(f"\n当前目标状态: {state_desc.get(self.current_state, '未知')}")


# 执行流程模板
class ExecutionTemplates:
    """常用执行流程模板"""

    @staticmethod
    def debug_boot_sequence():
        """调试启动序列"""
        return [
            "init",
            "reset halt",
            "reg",
            "# 此时目标已暂停，可以设置断点",
            "# 使用 bp 命令设置断点",
            "# 使用 resume 命令恢复执行"
        ]

    @staticmethod
    def single_step_debug():
        """单步调试流程"""
        return [
            "halt",
            "step",
            "reg",
            "# 查看寄存器状态",
            "step",
            "reg",
            "# 继续单步"
        ]

    @staticmethod
    def crash_analysis():
        """崩溃分析流程"""
        return [
            "halt",
            "reg",
            "# 查看 PC, LR, SP 寄存器",
            "mdw 0x20000000 16",
            "# 查看栈内容",
            "mdw 0xE000ED24 4",
            "# 查看 HFSR (硬故障状态寄存器)",
            "mdw 0xE000ED28 4",
            "# 查看 DFSR (调试故障状态寄存器)",
            "mdw 0xE000ED2C 4",
            "# 查看 AFSR (辅助故障状态寄存器)"
        ]

    @staticmethod
    def reset_and_run():
        """复位并运行流程"""
        return [
            "reset",
            "sleep 100",
            "# 等待复位完成",
            "resume"
        ]

    @staticmethod
    def flash_and_debug():
        """烧录后调试流程"""
        return [
            "# 1. 先烧录固件",
            "program build/Project.hex verify reset",
            "halt",
            "# 2. 设置断点",
            "bp 0x08000100",
            "# 3. 恢复执行",
            "resume"
        ]


# 故障寄存器地址
class STM32FaultRegisters:
    """STM32F103C8T6 故障诊断寄存器"""

    # 系统控制块 (SCB) 故障寄存器
    SCB_HFSR = 0xE000ED2C  # HardFault Status Register
    SCB_CFSR = 0xE000ED28  # Configurable Fault Status Register
    SCB_BFSR = 0xE000ED29  # Bus Fault Status Register (CFSR 低字节)
    SCB_UFSR = 0xE000ED2A  # Usage Fault Status Register (CFSR 字节 2-3)
    SCB_MMFSR = 0xE000ED28  # MemManage Fault Status Register (CFSR 高字节)
    SCB_MMFAR = 0xE000ED34  # MemManage Fault Address Register
    SCB_BFAR = 0xE000ED38  # Bus Fault Address Register

    @staticmethod
    def parse_hfsr(value):
        """解析 HardFault 状态寄存器"""
        print(f"\nHardFault 状态寄存器 (HFSR) = 0x{value:08X}")
        print("=" * 60)

        if value & (1 << 31):
            print("DEBUGEVT - 调试事件触发")

        if value & (1 << 30):
            print("FORCED - 强制 HardFault (CFSR 或 BFSR/UFAR/MMFSR 置位)")

        if value & (1 << 1):
            print("VECTTBL - 向量表读取错误")

        if value & (1 << 0):
            print("IMPDEFASSIST - IMPDEF 数据访问错误")

        if value == 0:
            print("无 HardFault")

        print("=" * 60)

    @staticmethod
    def parse_cfsr(value):
        """解析可配置故障状态寄存器"""
        print(f"\n可配置故障状态寄存器 (CFSR) = 0x{value:08X}")
        print("=" * 60)

        # MemManage Fault (bits 0-7)
        mmfsr = value & 0xFF
        if mmfsr:
            print("MemManage Fault:")
            if mmfsr & (1 << 7):
                print("  MMARVALID - MMFAR 有效")
            if mmfsr & (1 << 5):
                print("  MLSPERR - 访问时发生 MemManage 故障")
            if mmfsr & (1 << 4):
                print("  MSTKERR - 入栈时发生 MemManage 故障")
            if mmfsr & (1 << 3):
                print("  MUNSTKERR - 出栈时发生 MemManage 故障")
            if mmfsr & (1 << 1):
                print("  DACCVIOL - 数据访问违规")
            if mmfsr & (1 << 0):
                print("  IACCVIOL - 指令访问违规")

        # Bus Fault (bits 8-15)
        bfsr = (value >> 8) & 0xFF
        if bfsr:
            print("Bus Fault:")
            if bfsr & (1 << 7):
                print("  BFARVALID - BFAR 有效")
            if bfsr & (1 << 5):
                print("  LSPERR - 访问时发生 Bus Fault")
            if bfsr & (1 << 4):
                print("  STKERR - 入栈时发生 Bus Fault")
            if bfsr & (1 << 3):
                print("  UNSTKERR - 出栈时发生 Bus Fault")
            if bfsr & (1 << 2):
                print("  IMPRECISERR - 不精确的 Bus Fault")
            if bfsr & (1 << 1):
                print("  PRECISERR - 精确的 Bus Fault")
            if bfsr & (1 << 0):
                print("  IBUSERR - 指令总线错误")

        # Usage Fault (bits 16-31)
        ufsr = (value >> 16) & 0xFFFF
        if ufsr:
            print("Usage Fault:")
            if ufsr & (1 << 9):
                print("  DIVBYZERO - 除以零")
            if ufsr & (1 << 8):
                print("  UNALIGNED - 非对齐访问")
            if ufsr & (1 << 3):
                print("  NOCP - 无协处理器指令")
            if ufsr & (1 << 2):
                print("  INVPC - 无效的 PC 加载")
            if ufsr & (1 << 1):
                print("  INVSTATE - 无效的状态")
            if ufsr & (1 << 0):
                print("  UNDEFINSTR - 未定义指令")

        if value == 0:
            print("无 MemManage, Bus 或 Usage Fault")

        print("=" * 60)
