#ifndef BRADYPOS_ROOT_BUS_FABRIC_H
#define BRADYPOS_ROOT_BUS_FABRIC_H

#include <stdint.h>

/**
 * The RP2040 has some special functions in its bus fabric.
 * In particular, for MMIO, there are special alias addresses that will perform
 * atomic bit operations on the target instead of plain writes.
 * In particular, one can set bits, perform XORs or clear bits.
 */

enum
{
    REG_ALIAS_SET_OFFSET = 0x2000,
    REG_ALIAS_CLR_OFFSET = 0x3000,
    REG_ALIAS_XOR_OFFSET = 0x1000,
};

static inline void mmio_write32(uintptr_t addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
}
static inline void mmio_set32(uintptr_t addr, uint32_t val)
{
    *(volatile uint32_t *)(addr + REG_ALIAS_SET_OFFSET) = val;
}
static inline void mmio_clear32(uintptr_t addr, uint32_t val)
{
    *(volatile uint32_t *)(addr + REG_ALIAS_CLR_OFFSET) = val;
}
static inline void mmio_xor32(uintptr_t addr, uint32_t val)
{
    *(volatile uint32_t *)(addr + REG_ALIAS_XOR_OFFSET) = val;
}
static inline uint32_t mmio_read32(uintptr_t addr)
{
    return *(volatile uint32_t *)addr;
}
static inline void mmio_write_masked32(uintptr_t addr, uint32_t val,
                                       uint32_t mask)
{
    mmio_xor32(addr, (mmio_read32(addr) ^ val) & mask);
}
static inline volatile uint32_t *mmio_addr32(uintptr_t addr)
{
    return (volatile uint32_t *)addr;
}

#endif
