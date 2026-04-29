"""
Register Operator Module

处理 STM32F103C8T6 的寄存器读写操作。
包括核心寄存器（Cortex-M3）和外设寄存器。
"""


class RegisterOperator:
    """寄存器操作类"""

    # Cortex-M3 核心寄存器组
    CORE_REGISTERS = {
        # 通用寄存器
        "r0": {"desc": "General Purpose Register 0", "size": 32},
        "r1": {"desc": "General Purpose Register 1", "size": 32},
        "r2": {"desc": "General Purpose Register 2", "size": 32},
        "r3": {"desc": "General Purpose Register 3", "size": 32},
        "r4": {"desc": "General Purpose Register 4", "size": 32},
        "r5": {"desc": "General Purpose Register 5", "size": 32},
        "r6": {"desc": "General Purpose Register 6", "size": 32},
        "r7": {"desc": "General Purpose Register 7", "size": 32},
        "r8": {"desc": "General Purpose Register 8", "size": 32},
        "r9": {"desc": "General Purpose Register 9", "size": 32},
        "r10": {"desc": "General Purpose Register 10", "size": 32},
        "r11": {"desc": "General Purpose Register 11", "size": 32},
        "r12": {"desc": "General Purpose Register 12", "size": 32},

        # 特殊寄存器
        "sp": {"desc": "Stack Pointer (R13)", "size": 32},
        "lr": {"desc": "Link Register (R14)", "size": 32},
        "pc": {"desc": "Program Counter (R15)", "size": 32},
        "xpsr": {"desc": "Combined Program Status Register", "size": 32},

        # 程序状态寄存器
        "apsr": {"desc": "Application Program Status Register", "size": 32},
        "ipsr": {"desc": "Interrupt Program Status Register", "size": 32},
        "epsr": {"desc": "Execution Program Status Register", "size": 32},

        # 控制寄存器
        "primask": {"desc": "Priority Mask Register", "size": 32},
        "basepri": {"desc": "Base Priority Mask Register", "size": 32},
        "faultmask": {"desc": "Fault Mask Register", "size": 32},
        "control": {"desc": "Control Register", "size": 32},

        # 浮点寄存器（如果支持 FPU）
        "fpscr": {"desc": "Floating-Point Status and Control Register", "size": 32},

        # 其他
        "msp": {"desc": "Main Stack Pointer", "size": 32},
        "psp": {"desc": "Process Stack Pointer", "size": 32},
    }

    # 外设寄存器映射（GPIO）
    GPIO_REGISTERS = {
        "CRL": {"offset": 0x00, "desc": "Port Configuration Register Low"},
        "CRH": {"offset": 0x04, "desc": "Port Configuration Register High"},
        "IDR": {"offset": 0x08, "desc": "Port Input Data Register"},
        "ODR": {"offset": 0x0C, "desc": "Port Output Data Register"},
        "BSRR": {"offset": 0x10, "desc": "Port Bit Set/Reset Register"},
        "BRR": {"offset": 0x14, "desc": "Port Bit Reset Register"},
        "LCKR": {"offset": 0x18, "desc": "Port Configuration Lock Register"},
    }

    # 外设基地址
    PERIPHERAL_BASES = {
        "GPIOA": 0x40010800,
        "GPIOB": 0x40010C00,
        "GPIOC": 0x40011000,
        "USART1": 0x40013800,
        "USART2": 0x40004400,
        "TIM1": 0x40012C00,
        "TIM2": 0x40000000,
        "TIM3": 0x40000400,
        "TIM4": 0x40000800,
        "I2C1": 0x40005400,
        "SPI1": 0x40013000,
        "ADC1": 0x40012400,
        "DMA": 0x40020000,
    }

    def __init__(self):
        pass

    def show_registers(self, reg_name=None):
        """
        生成显示寄存器的 OpenOCD 命令

        Args:
            reg_name: 寄存器名称（None 显示所有寄存器）

        Returns:
            OpenOCD 命令列表
        """
        if reg_name is None:
            return ["reg"]
        else:
            return [f"reg {reg_name}"]

    def write_register(self, reg_name, value):
        """
        生成写入寄存器的 OpenOCD 命令

        Args:
            reg_name: 寄存器名称
            value: 写入值

        Returns:
            OpenOCD 命令列表
        """
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

        return [f"reg {reg_name} {value_hex}"]

    def parse_output(self, output):
        """
        解析 OpenOCD 寄存器输出

        Args:
            output: OpenOCD 标准输出
        """
        print(f"\n{'='*60}")
        print("寄存器状态")
        print(f"{'='*60}\n")

        lines = output.strip().split("\n")

        # 解析每一行
        for line in lines:
            line = line.strip()
            if ":" in line and not line.startswith("target"):
                # 格式: r0 /0x12345678
                parts = line.split()

                if len(parts) >= 2:
                    reg_name = parts[0]
                    reg_value = parts[1]

                    # 获取寄存器描述
                    desc = self.CORE_REGISTERS.get(reg_name.lower(), {}).get("desc", "")

                    # 解析值
                    if reg_value.startswith("/"):
                        reg_value = reg_value[1:]

                    if reg_value.startswith("0x"):
                        try:
                            int_val = int(reg_value, 16)
                            print(f"{reg_name:>8s} = {reg_value:<12s} ({int_val:>12d})  {desc}")

                            # 对于某些寄存器，显示位域信息
                            if reg_name.lower() == "xpsr":
                                self._parse_xpsr(int_val)
                            elif reg_name.lower() == "pc":
                                print(f"        -> 执行地址")
                            elif reg_name.lower() == "sp":
                                print(f"        -> 栈指针")
                            elif reg_name.lower() == "lr":
                                print(f"        -> 返回地址")

                        except ValueError:
                            print(f"{reg_name:>8s} = {reg_value}")

    def _parse_xpsr(self, xpsr_value):
        """解析 XPSR 寄存器的位域"""
        print(f"\n        XPSR 位域:")

        # N (Negative) flag - bit 31
        if xpsr_value & (1 << 31):
            print(f"          N = 1  (Negative)")

        # Z (Zero) flag - bit 30
        if xpsr_value & (1 << 30):
            print(f"          Z = 1  (Zero)")

        # C (Carry) flag - bit 29
        if xpsr_value & (1 << 29):
            print(f"          C = 1  (Carry)")

        # V (Overflow) flag - bit 28
        if xpsr_value & (1 << 28):
            print(f"          V = 1  (Overflow)")

        # T (Thumb state) bit - bit 24
        if xpsr_value & (1 << 24):
            print(f"          T = 1  (Thumb Mode)")
        else:
            print(f"          T = 0  (ARM Mode)")

        # ISR number - bits 0-8
        isr_num = xpsr_value & 0x1FF
        if isr_num != 0:
            print(f"          ISR = {isr_num}")
        else:
            print(f"          ISR = 0 (Thread Mode)")

    def show_peripheral_register(self, peripheral, reg_name):
        """
        生成读取外设寄存器的内存命令

        Args:
            peripheral: 外设名称（如 GPIOA, USART1）
            reg_name: 寄存器名称（如 CRL, CRH）

        Returns:
            OpenOCD 命令列表
        """
        base = self.PERIPHERAL_BASES.get(peripheral.upper())
        if base is None:
            raise ValueError(f"未知外设: {peripheral}")

        offset = self.GPIO_REGISTERS.get(reg_name.upper(), {}).get("offset")
        if offset is None:
            raise ValueError(f"未知寄存器: {reg_name}")

        addr = base + offset
        return [f"mdw 0x{addr:X}"]

    def show_all_peripheral_registers(self, peripheral):
        """
        生成读取外设所有寄存器的命令

        Args:
            peripheral: 外设名称

        Returns:
            OpenOCD 命令列表
        """
        base = self.PERIPHERAL_BASES.get(peripheral.upper())
        if base is None:
            raise ValueError(f"未知外设: {peripheral}")

        # GPIO 寄存器
        if peripheral.upper().startswith("GPIO"):
            commands = []
            for reg_name, info in self.GPIO_REGISTERS.items():
                addr = base + info["offset"]
                commands.append(f"mdw 0x{addr:X}")
            return commands

        # 其他外设：读取前 16 个寄存器（64 字节）
        commands = []
        for i in range(16):
            addr = base + i * 4
            commands.append(f"mdw 0x{addr:X}")
        return commands

    def get_register_info(self, reg_name):
        """
        获取寄存器信息

        Args:
            reg_name: 寄存器名称

        Returns:
            寄存器信息字典
        """
        return self.CORE_REGISTERS.get(reg_name.lower(), None)

    def list_all_registers(self):
        """列出所有已知的核心寄存器"""
        print("\nCortex-M3 核心寄存器列表:")
        print("=" * 60)

        for reg_name, info in self.CORE_REGISTERS.items():
            print(f"{reg_name:>12s} | {info['size']:>3} bits | {info['desc']}")

        print("=" * 60)

    def list_peripheral_registers(self, peripheral):
        """列出外设寄存器"""
        print(f"\n{peripheral} 寄存器:")
        print("=" * 60)

        if peripheral.upper().startswith("GPIO"):
            for reg_name, info in self.GPIO_REGISTERS.items():
                print(f"{reg_name:>8s} | +0x{info['offset']:02X} | {info['desc']}")
        else:
            print("使用 mdw 命令直接读取外设寄存器地址")

        print("=" * 60)

    def monitor_register(self, reg_name):
        """
        生成持续监控寄存器的命令

        Args:
            reg_name: 寄存器名称

        Returns:
            OpenOCD 命令列表
        """
        return [
            f"reg {reg_name}",
            "# 使用 GDB 监控模式: monitor mdw <addr>"
        ]

    # 外设寄存器位域解析
    def parse_gpio_crl(self, value):
        """解析 GPIO CRL 寄存器（低 8 位配置）"""
        print(f"\nGPIO CRL 配置:")
        print("=" * 50)

        for i in range(8):
            cnf = (value >> (i * 4)) & 0x3
            mode = (value >> (i * 4 + 2)) & 0x3

            cnf_desc = ["输入", "输出", "复用", "模拟"][cnf]
            mode_desc = ["输入", "10MHz", "2MHz", "50MHz"][mode]

            print(f"  GPIO{i}: CNF={cnf}({cnf_desc}), MODE={mode}({mode_desc})")

        print("=" * 50)

    def parse_gpio_crh(self, value):
        """解析 GPIO CRH 寄存器（高 8 位配置）"""
        print(f"\nGPIO CRH 配置:")
        print("=" * 50)

        for i in range(8):
            cnf = (value >> (i * 4)) & 0x3
            mode = (value >> (i * 4 + 2)) & 0x3

            cnf_desc = ["输入", "输出", "复用", "模拟"][cnf]
            mode_desc = ["输入", "10MHz", "2MHz", "50MHz"][mode]

            print(f"  GPIO{i+8}: CNF={cnf}({cnf_desc}), MODE={mode}({mode_desc})")

        print("=" * 50)

    def parse_gpio_idr(self, value):
        """解析 GPIO IDR 寄存器（输入数据）"""
        print(f"\nGPIO 输入状态:")
        print("=" * 50)

        for i in range(16):
            pin_value = (value >> i) & 0x1
            state = "HIGH" if pin_value else "LOW"
            print(f"  GPIO{i:2d}: {state}")

        print("=" * 50)

    def parse_gpio_odr(self, value):
        """解析 GPIO ODR 寄存器（输出数据）"""
        print(f"\nGPIO 输出状态:")
        print("=" * 50)

        for i in range(16):
            pin_value = (value >> i) & 0x1
            state = "HIGH" if pin_value else "LOW"
            print(f"  GPIO{i:2d}: {state}")

        print("=" * 50)


# 预定义的寄存器地址
class STM32Registers:
    """STM32F103C8T6 寄存器地址定义"""

    # 系统控制块 (SCB)
    SCB_CPUID = 0xE000ED00
    SCB_ICSR = 0xE000ED04
    SCB_VTOR = 0xE000ED08
    SCB_AIRCR = 0xE000ED0C
    SCB_SCR = 0xE000ED10
    SCB_CCR = 0xE000ED14

    # 嵌套向量中断控制器 (NVIC)
    NVIC_ISER = 0xE000E100
    NVIC_ICER = 0xE000E180
    NVIC_ISPR = 0xE000E200
    NVIC_ICPR = 0xE000E280
    NVIC_IABR = 0xE000E300
    NVIC_IP = 0xE000E400

    # SysTick
    SYSTICK_CSR = 0xE000E010
    SYSTICK_RVR = 0xE000E014
    SYSTICK_CVR = 0xE000E018
    SYSTICK_CALIB = 0xE000E01C

    # 时钟控制 (RCC)
    RCC_CR = 0x40021000
    RCC_CFGR = 0x40021004
    RCC_CIR = 0x40021008
    RCC_APB2RSTR = 0x4002100C
    RCC_APB1RSTR = 0x40021010
    RCC_AHBENR = 0x40021014
    RCC_APB2ENR = 0x40021018
    RCC_APB1ENR = 0x4002101C
    RCC_BDCR = 0x40021020
    RCC_CSR = 0x40021024

    # PWR 控制
    PWR_CR = 0x40007000
    PWR_CSR = 0x40007004

    @staticmethod
    def get_nvic_enable_irq(irq_num):
        """获取 NVIC 使能中断寄存器地址和位位置"""
        reg_offset = (irq_num // 32) * 4
        bit_pos = irq_num % 32
        return STM32Registers.NVIC_ISER + reg_offset, bit_pos

    @staticmethod
    def get_rcc_apb2enr(peripheral):
        """获取 RCC APB2 外设使能位位置"""
        bits = {
            "AFIO": 0,
            "IOPA": 2,
            "IOPB": 3,
            "IOPC": 4,
            "IOPD": 5,
            "IOPE": 6,
            "IOPF": 7,
            "IOPG": 8,
            "ADC1": 9,
            "ADC2": 10,
            "TIM1": 11,
            "SPI1": 12,
            "TIM8": 13,
            "USART1": 14,
            "ADC3": 15,
        }
        return bits.get(peripheral.upper())
