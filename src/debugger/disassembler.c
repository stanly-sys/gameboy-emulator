/**
 * @file disassembler.c
 * @brief Formats the next instruction at an address (peek, no side effects).
 */

#include "gb/gb.h"

#include <stdio.h>

extern const char *const gb_op_mnemonics[256];
extern const u8 gb_op_size[256];

int disassemble_at(gb_t *gb, u16 addr, char *out, size_t out_cap)
{
    if (out == NULL || out_cap == 0) {
        return 0;
    }
    const u8 op = mmu_read(gb, addr);
    if (op == 0xCB) {
        const u8 cb = mmu_read(gb, (u16)(addr + 1u));
        return snprintf(out, out_cap, "%04X: CB %02X", addr, cb);
    }
    const u8 n = gb_op_size[op];
    if (n == 2) {
        return snprintf(out, out_cap, "%04X: %02X %02X  %s", addr, op, mmu_read(gb, (u16)(addr + 1u)),
                        gb_op_mnemonics[op]);
    }
    if (n == 3) {
        return snprintf(out, out_cap, "%04X: %02X %02X %02X  %s", addr, op,
                        mmu_read(gb, (u16)(addr + 1u)), mmu_read(gb, (u16)(addr + 2u)),
                        gb_op_mnemonics[op]);
    }
    return snprintf(out, out_cap, "%04X: %02X        %s", addr, op, gb_op_mnemonics[op]);
}
