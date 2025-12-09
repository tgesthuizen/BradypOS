#ifndef BRADYPOS_ROOT_BUS_FABRIC_H
#define BRADYPOS_ROOT_BUS_FABRIC_H

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

#endif
