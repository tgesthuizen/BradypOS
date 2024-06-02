	.cpu cortex-m0plus
	.thumb
	.section ".data.kip", "dwa"
	.align 8
	.global the_kip, __L4_boot_info
	.hidden the_kip
	.type the_kip, %object
the_kip:
	.ascii "L4\x230K"
.Lkip_api_version:
	.word (0x84 << 24) | (7 << 16)
.Lkip_api_flags:
	.word 0
.Lkip_kern_desc_ptr:
	.word kern_desc - the_kip

	.zero 0x44
.Lkip_memory_info:
	.word 0 /* Memory info, needs dynamic initialization */

	.zero 0x50
	// Minimal UTCB area size is 512 bytes, UTCBs are 2^3 byte
	// aligned and are at least 160 byte large
	.word (9 << 16) | (3 << 10) | (160 / 4)
	.word 0xA0 /* KIP is 1024 bytes */
	.zero 0x8
	.word __L4_boot_info - the_kip
	.word proc_desc - the_kip
	.word 0 /* TODO: clock info */
	.word (0x40 << 20) | (0x30 << 8) | (16 << 0) /* Thread info */
	.word ~((1 << 11) - 1) | 0b111 /* Page info: all rights changeable, all page sizes supported */
	.word (4 << 28) | (0 << 0) /* Processor info: 1 Processor (for now), each record is 2^4 */

.Lkip_syscalls:
	.zero 0x2c
	.zero 0x04

	.size the_kip,.-the_kip

	.type kern_desc, %object
	.hidden kern_desc
kern_desc:
	.word (42 << 24) | (1 << 16) /* Kernel version */
	.word (24 << 9) | (5 << 4) | (1 << 0) /* Kernel gen date */
	.asciz "tim" /* Kernel supplier, idk. */
	.asciz "BradypOS", "kern", ""
	.size kern_desc, .-kern_desc

	.type proc_desc, %object
	.hidden proc_desc
proc_desc:
	.word 256 * 1024 * 1024
	.word 256 * 1024 * 1024
	.size proc_desc,.-proc_desc

	.align 3
__L4_boot_info:
	# This gets written by the boot loader

	# Fill the rest of the page
	.fill (the_kip + 1024) - .

	/* HACK: This is required to reliably link with libl4 at the
	 * moment, but should be solved properly instead
	 */
.section ".rodata", "da"
	.global __utcb
	.set __utcb, 0
