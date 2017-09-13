# 错误的 IRQ 中断路由及修复

## 问题描述

在 Lenovo Yoga Y13 的 USB 3.0 上插设备无法识别，且导致系统图形性能下降。
journal 中出现：

    irq 16: nobody cared (try booting with the "irqpoll" option)
    Disabling IRQ #16

打开 `noapic` 或者 `irqpoll` 则一切正常，但很明显它们将降低系统效率。

## 问题分析

`cat /proc/interrupts`:

     16:      85932     284829      48555      35311   IO-APIC  16-fasteoi   i915, ehci_hcd:usb1

可是我们插的设备在 USB 3.0 (xHCI) 控制器上，而不是 EHCI 。然而：

     21:          0          0          0          0   IO-APIC  21-fasteoi   xhci_hcd:usb3

可见，系统误认为 USB 3.0 控制器送出 IRQ 21 ，收到 IRQ 16 时根本没有考虑这
可能是 USB 3.0 控制器的中断。使用 irqpoll 时系统会试一下 `xhci_hcd` 的中断
处理程序，从而解决问题。使用 `noapic` 时系统不使用 IO-APIC ，而是使用 8259，
中断路由表是正常的。

无法处理的中断导致该中断被禁掉，于是 `i915` 无法处理中断，显示性能急剧下降。

## 问题解决

查资料知道中断路由表来自 ACPI ，用 `acpidump` 获取 ACPI 数据，以
`acpixtract` 分解出各个表。中断信息在 DSDT 表中，将其反编译：

    $ acpidump > acpi.dump
    $ acpixtract acpi.dump
    $ iasl -d dsdt.dat

产生 DSDT 表的源代码 `dsdt.dsl` 。

找到 xHCI 控制器的 PCI 地址：

    $ lspci | grep xHCI
    00:14.0 USB controller: Intel Corporation 7 Series/C210 Series Chipset Family USB xHCI Host Controller (rev 04)

打开 `dsdt.dsl` ，首先把版本号增大，以保证内核使用修改过的 DSDT：

    DefinitionBlock ("", "DSDT", 1, "LENOVO", "CB-01   ", 0x00000001)

把最后的版本号改为 0x00000002 。 找到 DSDT 中的 PCI 路由表 `_PRT`：

    Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
    {
    	If (PICM)
    	{
    		Return (AR00) /* \_SB_.PCI0.AR00 */
		}

		Return (PR00) /* \SB_.PCI0.PR00 */
	}

PICM 被初始化为：

    Method (_PIC, 1, NotSerialized)  // _PIC: Interrupt Model
    {
    	GPIC = Arg0
    	PCIM = Arg0
	}

查 ACPI 标准知道，若启用了 IO-APIC ，则 `_PIC(1)` 被调用。可见， `AR00` 包
含了使用 IO-APIC 时的路由表。在该表找到 `00:14.0` 对应的项：

    Name (AR00, Package (0x25)
    {
    	Package (0x04)
    	{
    		0x0014FFFF,
    		Zero,
    		Zero,
    		0x17
		},
		....
	})

`0x17` 就是 IRQ 21 。将其改为 16 (`0x10`) 。用 `iasl` 编译改好的 DSDT ，结
果有 Error ：

    Local0 = Arg0
    (Local0 & 0x20)
    Local0 >>= 0x05
    BFUC = Local0
    Return (Local0)

错误提示为 `(Local0 & 0x20)` 的结果未使用。根据经验猜测此处应为

    Local0 = Arg0 & 0x20
    Local0 >>= 0x05
    ...

修改后再次编译：

	$ iasl dsdt.dsl

得到 `dsdt.aml` 字节码文件。

下面让内核加载修改的 DSDT 。创建一个 `cpio` 镜像：

    $ mkdir -pv kernel/firmware/acpi
    $ cp dsdt.aml kernel/firmware/acpi
    $ find kernel | cpio -H newc --create > acpi_override
    $ sudo cp acpi_override /boot

编辑 `grub.cfg` 使其加载该 `cpio` 镜像作为初始化内存盘：

    linux /boot/vmlinuz root=/dev/sda2 ro
    initrd /boot/acpi_override

重启。然后测试是否加载了新的 DSDT:

    $ dmesg | grep DSDT
    [    0.000000] ACPI: DSDT ACPI table found in initrd [kernel/firmware/acpi/dsdt.aml][0x882b]
    [    0.000000] ACPI: Table Upgrade: override [DSDT-LENOVO-CB-01   ]
    [    0.000000] ACPI: DSDT 0x00000000CAFE9000 Physical table override, new table: 0x00000000CA42C000
    [    0.000000] ACPI: DSDT 0x00000000CA42C000 00882B (v01 LENOVO CB-01    00000002 INTL 20170831)

测试中断路由：

    $ cat /proc/interrupts | grep xhci
     16:     103870     410757      61790      46295   IO-APIC  16-fasteoi   i915, ehci_hcd:usb1, xhci-hcd:usb3

然后再插上 USB 3.0 设备就没问题了。

## 吐槽

1. 生产商居然能在 BIOS 里面把中断都搞错。（难道当时的系统不用 APIC？）

2. 建议 `irqpoll` 可以在 journal 里面输出尝试出来的 "奇怪" 中断处理，以方便
调试。 （或许本来就有这功能，只是我不知道？）
