	.section ".data", "dwa"
	.align 4
	.global __stack, __sp
	.hidden __stack
	.type __stack, %object
__stack:
	.space 512
__sp:
	.size __stack,.-__stack

	.align 9
	.global __utcb
	.type __utcb, %object
__utcb:
	.space 512
	.size __utcb,.-__utcb
