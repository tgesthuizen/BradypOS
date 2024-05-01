#ifndef HARDWARE_H
#define HARDWARE_H

#define PPB_BASE 0xe0000000

#define ACTLR_OFFSET 0xe008
#define CPUID_OFFSET 0xed00
#define ICSR_OFFSET 0xed04
#define VTOR_OFFSET 0xed08
#define AIRCR_OFFSET 0xed0c
#define SCR_OFFSET 0xed10
#define CCR_OFFSET 0xed14
#define SHPR2_OFFSET 0xed1c
#define SHPR3_OFFSET 0xed20
#define SHCSR_OFFSET 0xed24
#define DFSR_OFFSET 0xed30

#define SYST_CSR 0xe010
#define SYST_RVR 0xe014
#define SYST_CVR 0xe018
#define SYST_CALIB 0xe01c

#define IO_BANK0_BASE 0x40014000
#define GPIO_BLOCK_SIZE 0x8
#define TARGET_GPIO 25
#define PIN_FUNC_SIO 0x5

#define SIO_BASE 0xd0000000
#define SIO_GPIO_OUT_SET 0x014
#define SIO_GPIO_OUT_XOR 0x01c

#endif
