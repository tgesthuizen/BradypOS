	.cpu cortex-m0plus
	.syntax unified

	.global kern_stack_base, kern_stack_size
	.hidden kern_stack_base, kern_stack_size
	.set kern_stack_base, 0x20040000
	.set kern_stack_size, 0x200

	.section ".text", "ax"
	.global _start
	.hidden _start
	.func _start
	.thumb_func
_start:
	cpsid i
	movs  r4, #0
	mov   lr, r4
	# Store r9 at the root of the stack for restoring by IRQs
	ldr   r5, =kern_stack_base
	mov   r6, r9
	str   r6, [r5]
	ldr   r5, =(kern_stack_base + kern_stack_size)
	msr   msp, r5
	msr   control, r4
	isb

	ldr   r4, .Lroot_stack_got
	ldr   r5, .Lroot_utcb_got
	ldr   r4, [r6, r4]
	str   r0, [r4]
	ldr   r5, [r6, r5]
	str   r1, [r5]
	ldr   r4, .Lroot_got_got
	str   r2, [r6, r4]
	
	bl    main
	
busy_loop:
	wfi
	b   busy_loop

	.align 2
.Lroot_stack_got:
	.word root_stack(GOT)
.Lroot_utcb_got:
	.word root_utcb(GOT)
.Lroot_got_got:
	.word root_got(GOT)
	.pool
	.endfunc
