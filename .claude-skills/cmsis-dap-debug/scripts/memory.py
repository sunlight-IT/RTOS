"""
Memory Operator Module

处理 STM32F103C8T6 的内存读写操作。
使用 OpenOCD 的 mdw/mdh/mdb 和 mww/mwh/mwb 命令。
"""


class MemoryOperator:
    """内存操作类"""

    # STM32F103C8T6 内存映射
    MEMORY_MAP = {
        "FLASH": {
            "start": 0x08000000,
            "end": 0x08010000,
            "size": 0x10000,
            "desc": "64KB FLASH Memory",
            "access": "rx",  # read, execute
        },
        "RAM": {
            "start": 0x20000000,
            "end": 0x20005000,
            "size": 0x5000,
            "desc": "20KB SRAM",
            "access": "rw",  # read, write
        },
        "PERIPHERAL": {
            "start": 0x40000000,
            "end": 0x60000000,
            "size": 0x20000000,
            "desc": "Peripheral Registers (512MB)",
            "access": "rw",
        },
        "SYSTEM": {
            "start": 0xE0000000,
            "end": 0xE0100000,
            "size": 0x100000,
            "desc": "System Control Space (Cortex-M3)",
            "access": "rw",
        },
    }

    def __init__(self):
        pass

    def read_memory(self, addr, count=1, size="w"):
        """
        生成读取内存的 OpenOCD 命令

        Args:
            addr: 起始地址（支持 0x 或十进制）
            count: 读取数量
            size: 数据大小
                   - "w": 32-bit word (4 bytes)
                   - "h": 16-bit halfword (2 bytes)
                   - "b": 8-bit byte

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
        cmd = f"md{size} {addr_hex} {count}"
        return [cmd]

    def write_memory(self, addr, value, size="w"):
        """
        生成写入内存的 OpenOCD 命令

        Args:
            addr: 目标地址
            value: 写入的值
            size: 数据大小
                   - "w": 32-bit word
                   - "h": 16-bit halfword
                   - "b": 8-bit byte

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

        # 确保值格式正确
        if isinstance(value, int):
            value_hex = f"0x{value:X}"
        elif isinstance(value, str):
            if value.startswith("0x"):
                value_hex = value
            else:
                value_hex = f"0x{int(value, 0):X}"
        else:
            raise ValueError(f"无效的值格式: {value}")

        # 生成命令
        cmd = f"mw{size} {addr_hex} {value_hex}"
        return [cmd]

    def parse_output(self, output, base_addr, count, size="w"):
        """
        解析 OpenOCD 内存读取输出

        Args:
            output: OpenOCD 标准输出
            base_addr: 基地址
            count: 读取数量
            size: 数据大小
        """
        # 转换地址为整数
        if isinstance(base_addr, str):
            addr_int = int(base_addr, 0) if base_addr.startswith("0x") else int(base_addr)
            addr_display = base_addr
        else:
            addr_int = base_addr
            addr_display = f"0x{base_addr:X}"

        print(f"\n{'='*50}")
        print(f"内存读取 @ {addr_display} ({count} x {size})")
        print(f"{'='*50}")

        # 确定数据类型
        size_map = {"b": "Byte", "h": "Halfword", "w": "Word"}
        data_type = size_map.get(size, "Unknown")

        lines = output.strip().split("\n")

        for line in lines:
            line = line.strip()
            if line and (":" in line or "0x" in line):
                print(line)

        print(f"\n[INFO] 数据类型: {data_type}")
        print(f"[INFO] 读取地址: {addr_display}")
        print(f"[INFO] 读取数量: {count}")

        # 尝试解析数值
        values = []
        for line in lines:
            if ":" in line:
                parts = line.split(":")
                if len(parts) == 2:
                    addr_part = parts[0].strip()
                    data_part = parts[1].strip()
                    values.extend(data_part.split())

        if values:
            print(f"[INFO] 解析到 {len(values)} 个值")
            # 显示十六进制和十进制
            print("\n值列表:")
            for i, val in enumerate(values[:count]):
                if val.startswith("0x"):
                    int_val = int(val, 16)
                    print(f"  [{i:2d}] {val:>10s} = {int_val:>12d} = 0b{int_val:032b}")
                else:
                    int_val = int(val, 16) if len(val) == 8 else int(val)
                    print(f"  [{i:2d}] {val:>10s} = {int_val:>12d}")

    def get_memory_region(self, addr):
        """
        获取地址所属的内存区域

        Args:
            addr: 地址

        Returns:
            区域信息字典，如果不在任何区域则返回 None
        """
        addr_int = int(addr, 0) if isinstance(addr, str) else addr

        for name, region in self.MEMORY_MAP.items():
            if region["start"] <= addr_int < region["end"]:
                return region

        return None

    def print_memory_map(self):
        """打印内存映射"""
        print("\nSTM32F103C8T6 内存映射:")
        print("=" * 60)

        for name, region in self.MEMORY_MAP.items():
            print(f"{name:12s} | 0x{region['start']:08X} - 0x{region['end']:08X} | "
                  f"{region['size']:>8X} | {region['desc']:<30s}")

        print("=" * 60)

    def validate_address(self, addr, access_type="rw"):
        """
        验证地址是否有效且可访问

        Args:
            addr: 地址
            access_type: 访问类型 ("r", "w", "rw")

        Returns:
            (is_valid, region_name, error_msg)
        """
        region = self.get_memory_region(addr)

        if region is None:
            return False, None, f"地址 0x{int(addr, 0):X} 不在任何已知内存区域"

        if access_type == "w" or "w" in access_type:
            if "w" not in region["access"]:
                return False, region["desc"], f"{region['desc']} 不支持写入"

        if access_type == "r" or "r" in access_type:
            if "r" not in region["access"]:
                return False, region["desc"], f"{region['desc']} 不支持读取"

        return True, region["desc"], None

    def dump_flash_range(self, start_addr, end_addr=None, size_bytes=None):
        """
        导出 FLASH 指定范围的内容

        Args:
            start_addr: 起始地址
            end_addr: 结束地址（可选）
            size_bytes: 字节大小（可选，与 end_addr 二选一）

        Returns:
            命令列表
        """
        start = int(start_addr, 0) if isinstance(start_addr, str) else start_addr

        if end_addr is not None:
            end = int(end_addr, 0) if isinstance(end_addr, str) else end_addr
            size = end - start
        elif size_bytes is not None:
            size = size_bytes
        else:
            raise ValueError("必须指定 end_addr 或 size_bytes")

        # 计算需要读取的 word 数量
        word_count = (size + 3) // 4  # 向上取整

        return self.read_memory(start, word_count, "w")

    def fill_memory(self, start_addr, value, count, size="w"):
        """
        填充内存区域（写入相同的值）

        Args:
            start_addr: 起始地址
            value: 填充值
            count: 填充数量
            size: 数据大小

        Returns:
            命令列表
        """
        commands = []

        for i in range(count):
            addr = start_addr + i * (1 if size == "b" else 2 if size == "h" else 4)
            commands.extend(self.write_memory(addr, value, size))

        return commands

    def search_pattern(self, start_addr, end_addr, pattern, size="w"):
        """
        在内存中搜索模式（生成命令，实际搜索需要手动解析）

        Args:
            start_addr: 起始地址
            end_addr: 结束地址
            pattern: 搜索模式（十六进制字符串）
            size: 数据大小

        Returns:
            命令列表
        """
        start = int(start_addr, 0) if isinstance(start_addr, str) else start_addr
        end = int(end_addr, 0) if isinstance(end_addr, str) else end_addr

        word_count = (end - start + 3) // 4

        # 读取整个范围
        commands = self.read_memory(start, word_count, size)
        commands.append(f"# 搜索模式: {pattern}")

        return commands


# 预定义的常用内存地址
class STM32Addresses:
    """STM32F103C8T6 常用地址"""

    # 中断向量表
    VECTOR_TABLE = 0x08000000

    # 栈指针
    INITIAL_SP = 0x08000000

    # 复位向量
    RESET_HANDLER = 0x08000004

    # 系统存储器 (Bootloader)
    SYSTEM_MEM = 0x1FFFF000

    # 选项字节
    OPTION_BYTES = 0x1FFFF800

    # RAM 起始
    RAM_START = 0x20000000

    # 常用外设基地址
    GPIOA_BASE = 0x40010800
    GPIOB_BASE = 0x40010C00
    GPIOC_BASE = 0x40011000
    USART1_BASE = 0x40013800
    USART2_BASE = 0x40004400
    TIM1_BASE = 0x40012C00
    TIM2_BASE = 0x40000000

    # 核心寄存器 (SCS)
    SCB_BASE = 0xE000ED00
    NVIC_BASE = 0xE000E100
    SYSTICK_BASE = 0xE000E010

    @staticmethod
    def get_peripheral_address(peripheral, offset=0):
        """获取外设寄存器地址"""
        base = getattr(STM32Addresses, f"{peripheral}_BASE", None)
        if base is None:
            raise ValueError(f"未知外设: {peripheral}")
        return base + offset
