	.cpu cortex-m0plus
	.syntax unified

	.section ".text", "ax"
	.global _start
	.func _start
	.thumb_func
_start:
	cpsid i
	movs r0, #0
	mov lr, r0
	ldr r0, =__stack
	msr msp, r0
	msr psp, r0
	bl main
	
busy_loop:
	wfi
	b busy_loop

	.pool
	.endfunc
