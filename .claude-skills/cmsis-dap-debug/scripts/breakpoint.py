"""
Breakpoint Manager Module

处理 STM32F103C8T6 的断点管理操作。
使用 OpenOCD 的 bp, rbp 命令。
"""


class BreakpointManager:
    """断点管理类"""

    # STM32F103C8T6 断点资源
    # Cortex-M3 支持 6 个硬件断点
    MAX_HW_BREAKPOINTS = 6
    MAX_SW_BREAKPOINTS = 0  # Flash 中不支持软件断点

    def __init__(self):
        self.breakpoints = {}  # 存储已设置的断点

    def set_breakpoint(self, addr, hardware=False):
        """
        生成设置断点的 OpenOCD 命令

        Args:
            addr: 断点地址（支持 0x 前缀）
            hardware: 是否为硬件断点

        Returns:
            OpenOCD 命令列表
        """
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

        # 生成命令
        if hardware:
            cmd = f"bp {addr_hex} hw"
        else:
            cmd = f"bp {addr_hex}"

        return [cmd]

    def remove_breakpoint(self, bp_num):
        """
        生成移除断点的 OpenOCD 命令

        Args:
            bp_num: 断点编号

        Returns:
            OpenOCD 命令列表
        """
        return [f"rbp {bp_num}"]

    def list_breakpoints(self):
        """
        生成列出断点的 OpenOCD 命令

        Returns:
            OpenOCD 命令列表
        """
        return ["bp"]

    def parse_output(self, output):
        """
        解析 OpenOCD 断点输出

        Args:
            output: OpenOCD 标准输出
        """
        print(f"\n{'='*60}")
        print("断点列表")
        print(f"{'='*60}\n")

        lines = output.strip().split("\n")

        found_bp = False
        for line in lines:
            line = line.strip()

            # OpenOCD 格式: <num>: type at <addr>, <state>
            # 例如: 0: hw bp at 0x08000100, enabled
            if ":" in line and "bp" in line.lower():
                found_bp = True

                # 解析断点信息
                parts = line.split(":")

                if len(parts) >= 2:
                    bp_num = parts[0].strip()
                    bp_info = parts[1].strip()

                    # 提取地址
                    if "at" in bp_info:
                        addr_part = bp_info.split("at")[1].strip().split(",")[0]
                        print(f"断点 #{bp_num}: 地址 = {addr_part}")

                        # 判断类型
                        if "hw" in bp_info.lower():
                            print(f"        类型: 硬件断点")
                        else:
                            print(f"        类型: 软件断点")

                        # 判断状态
                        if "enabled" in bp_info.lower():
                            print(f"        状态: 已启用")
                        elif "disabled" in bp_info.lower():
                            print(f"        状态: 已禁用")

                        # 获取符号（如果有）
                        if "in" in bp_info:
                            func_part = bp_info.split("in")[1].strip()
                            print(f"        函数: {func_part}")

                        print()

        if not found_bp:
            print("当前没有设置任何断点")
            print()
            print(f"[INFO] STM32F103C8T6 支持最多 {self.MAX_HW_BREAKPOINTS} 个硬件断点")

    def set_conditional_breakpoint(self, addr, condition, hardware=False):
        """
        生成设置条件断点的 OpenOCD 命令

        注意：条件断点通常需要通过 GDB 设置

        Args:
            addr: 断点地址
            condition: 条件表达式
            hardware: 是否为硬件断点

        Returns:
            OpenOCD 命令列表
        """
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

        # 添加注释说明
        return [
            f"# 条件断点需要通过 GDB 设置",
            f"# 例如: break *{addr_hex} if {condition}",
            f"# OpenOCD 仅支持简单断点"
        ]

    def disable_breakpoint(self, bp_num):
        """
        生成禁用断点的 OpenOCD 命令

        Args:
            bp_num: 断点编号

        Returns:
            OpenOCD 命令列表
        """
        # OpenOCD 没有 disable 命令，需要先删除后设置
        return [
            f"# 禁用断点 #{bp_num}",
            f"# 使用 rbp {bp_num} 删除断点",
            f"# 重新设置时记录该断点"
        ]

    def clear_all_breakpoints(self):
        """
        生成清除所有断点的 OpenOCD 命令

        Returns:
            OpenOCD 命令列表
        """
        commands = []
        for i in range(self.MAX_HW_BREAKPOINTS):
            commands.append(f"rbp {i}")
        return commands

    def set_watchpoint(self, addr, size=4, access="rw"):
        """
        生成设置观察点的 OpenOCD 命令

        注意：观察点通过 GDB 设置

        Args:
            addr: 观察地址
            size: 观察大小（字节）
            access: 访问类型 (r=读, w=写, rw=读写)

        Returns:
            OpenOCD 命令列表
        """
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

        # Cortex-M3 支持数据观察点（使用 DWT）
        # 这里只生成注释，实际需要通过 GDB 设置
        return [
            f"# 观察点需要通过 GDB 设置",
            f"# 例如: watch *(uint32_t*)0x{addr_hex}",
            f"# 或者: rwatch *(uint32_t*)0x{addr_hex}  (仅读)",
            f"# 或者: awatch *(uint32_t*)0x{addr_hex}  (读/写)"
        ]

    def print_breakpoint_info(self):
        """打印断点资源信息"""
        print(f"\nSTM32F103C8T6 断点资源:")
        print("=" * 60)
        print(f"硬件断点: 最多 {self.MAX_HW_BREAKPOINTS} 个")
        print(f"软件断点: 不支持 (Flash 只读)")
        print(f"数据观察点: 支持 4 个 (通过 DWT)")
        print("=" * 60)

    def validate_breakpoint_address(self, addr):
        """
        验证断点地址是否有效

        Args:
            addr: 断点地址

        Returns:
            (is_valid, reason)
        """
        addr_int = int(addr, 0) if isinstance(addr, str) else addr

        # 检查是否在 Flash 区域
        if 0x08000000 <= addr_int < 0x08010000:
            return True, "在 Flash 区域"

        # 检查是否在 RAM 区域
        if 0x20000000 <= addr_int < 0x20005000:
            return True, "在 RAM 区域"

        return False, "地址不在可执行区域"

    def get_breakpoint_stats(self):
        """
        获取断点统计信息

        Returns:
            统计信息字典
        """
        return {
            "max_hw_breakpoints": self.MAX_HW_BREAKPOINTS,
            "max_sw_breakpoints": self.MAX_SW_BREAKPOINTS,
            "active_breakpoints": len(self.breakpoints),
        }


# 预定义的常用断点地址
class STM32Breakpoints:
    """STM32F103C8T6 常用断点地址"""

    # 异常向量表地址
    NMI_HANDLER = 0x08000008
    HARDFAULT_HANDLER = 0x0800000C
    MEMMANAGE_HANDLER = 0x08000010
    BUSFAULT_HANDLER = 0x08000014
    USAGEFAULT_HANDLER = 0x08000018

    # 常用函数地址（需要根据实际二进制文件确定）
    # 这些是示例地址，实际使用时需要调整
    MAIN_ENTRY = 0x08000000  # 通常在复位向量之后

    # 外设中断处理程序（需要根据向量表确定）
    EXTI0_IRQHandler = 0x08000000
    EXTI1_IRQHandler = 0x08000000

    @staticmethod
    def get_vector_table_address(vector_num):
        """
        获取向量表中指定项的地址

        Args:
            vector_num: 向量编号（0=SP, 1=Reset, 2=NMI, ...）

        Returns:
            向量地址
        """
        return 0x08000000 + vector_num * 4

    @staticmethod
    def parse_vector_table():
        """打印向量表信息"""
        print("\nSTM32F103C8T6 异常向量表:")
        print("=" * 60)

        vectors = [
            ("SP", 0, "初始栈指针"),
            ("Reset", 1, "复位处理程序"),
            ("NMI", 2, "不可屏蔽中断"),
            ("HardFault", 3, "硬件错误"),
            ("MemManage", 4, "内存管理错误"),
            ("BusFault", 5, "总线错误"),
            ("UsageFault", 6, "用法错误"),
            ("Reserved", 7, "保留"),
            ("SVCall", 8, "系统服务调用"),
            ("DebugMonitor", 9, "调试监控"),
            ("Reserved", 10, "保留"),
            ("PendSV", 11, "可挂起系统调用"),
            ("SysTick", 12, "系统滴答"),
        ]

        for name, num, desc in vectors:
            addr = STM32Breakpoints.get_vector_table_address(num)
            print(f"{name:>15s} @ 0x{addr:08X} (Vector #{num:2d}) - {desc}")

        print("=" * 60)
