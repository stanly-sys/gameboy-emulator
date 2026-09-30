/**
 * @file cpu_flags.c
 * @brief ZNHC flag helpers. Lower nibble of F is always forced to 0.
 *
 * Hardware reference: F register bits 7-4 only.
 * Cycle model: none.
 * Thread-safety: not thread-safe.
 * Error handling: none.
 */

#include "gb/gb.h"

void cpu_set_flags_znhc(cpu_t *cpu, bool z, bool n, bool h, bool c)
{
    u8 f = 0;
    if (z) {
        f = (u8)(f | GB_FLAG_Z);
    }
    if (n) {
        f = (u8)(f | GB_FLAG_N);
    }
    if (h) {
        f = (u8)(f | GB_FLAG_H);
    }
    if (c) {
        f = (u8)(f | GB_FLAG_C);
    }
    cpu->f = f;
}

bool cpu_flag(const cpu_t *cpu, u8 mask)
{
    return (cpu->f & mask) != 0;
}
