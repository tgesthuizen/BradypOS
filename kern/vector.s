	.cpu cortex-m0plus
	.thumb

	.text

	.type isr_invalid, %function
	.thumb_func
	.func
isr_invalid:
	bkpt #0
	b    .
	.endfunc

	.type irq_invalid, %function
	.thumb_func
	.func
irq_invalid:
	mrs  r0, ipsr
	sub  r0, #16
	bkpt #0
	b    .
	.endfunc

	.weakref isr_nmi, isr_invalid
	.weakref isr_hardfault, isr_invalid
	.weakref isr_svcall, isr_invalid
	.weakref isr_pendsv, isr_invalid
	.weakref isr_systick, isr_invalid
	.weakref handle_irq, irq_invalid

	.section ".data.vector", "aw"
	.align 7
	.global __vector
	.type __vector, %object
__vector:
	.word 0x20040000
	.word _start
	.word isr_nmi
	.word isr_hardfault
	.word isr_invalid
	.word isr_invalid
	.word isr_invalid
	.word isr_invalid
	.word isr_invalid
	.word isr_invalid
	.word isr_invalid
	.word isr_svcall
	.word isr_invalid
	.word isr_invalid
	.word isr_pendsv
	.word isr_systick
	.rept 32
	.word handle_irq
	.endr
	.size __vector, .-__vector
