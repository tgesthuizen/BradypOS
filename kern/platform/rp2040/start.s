	.cpu cortex-m0plus
	.syntax unified

	.section ".text", "ax"
	.global _start
	.func _start
	.thumb_func
_start:
	cpsid i
	movs  r1, #0
	mov   lr, r1
	ldr   r0, =0x20040000
	msr   msp, r0
	msr   psp, r0
	msr   control, r1
	isb
	bl    main
	
busy_loop:
	wfi
	b   busy_loop

	.pool
	.endfunc
