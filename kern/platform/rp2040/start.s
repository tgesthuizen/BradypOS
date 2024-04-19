	.cpu cortex-m0plus
	.syntax unified

	.section ".text", "ax"
	.global _start
	.func _start
	.thumb_func
_start:
	cpsid i
	movs  r0, #0
	mov   lr, r0
	ldr   r0, .Lstack_got_offset
	mov   r1, r9
	ldr   r0, [r1, r0]
	msr   msp, r0
	msr   psp, r0
	bl    main
	
busy_loop:
	wfi
	b   busy_loop

	.align 2
.Lstack_got_offset:
	.word __stack(GOT)
	.pool
	.endfunc
