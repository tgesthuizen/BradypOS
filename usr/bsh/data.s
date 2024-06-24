	.cpu cortex-m0plus

	.section ".data", "aw"
	.global __sp, __utcb
	
	.align 5
	.type stack, %object
stack:
	.space 512
	.size stack,.-stack
__sp:
	.align 9
	.type __utcb, %object
__utcb:	
	.space 512
	.size __utcb, .-__utcb
