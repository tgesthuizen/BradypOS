	.cpu cortex-m0plus
	.syntax unified

	.global kern_stack_base, kern_stack_size
	.hidden kern_stack_base, kern_stack_size
	.set kern_stack_base, 0x20040000
	.set kern_stack_size, 0x100
	
	.section ".text", "ax"
	.global _start
	.hidden _start
	.func _start
	.thumb_func
_start:
	cpsid i
	movs  r1, #0
	mov   lr, r1
	# Store r9 at the root of the stack for restoring by IRQs
	ldr   r0, =kern_stack_base
	mov   r2, r9
	str   r2, [r0]
	ldr   r0, =(kern_stack_base + kern_stack_size)
	msr   msp, r0
	msr   control, r1
	isb
	bl    main
	
busy_loop:
	wfi
	b   busy_loop

	.pool
	.endfunc
