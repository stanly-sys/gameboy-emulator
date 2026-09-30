/**
 * @file apu.c
 * @brief Register-accurate stub. Mixes silence so mobile/desktop share one core.
 *
 * Hardware reference: APU powered by NR52 bit 7.
 * Cycle model: ignored until channels are implemented.
 * Thread-safety: not thread-safe.
 * Error handling: reads of unused bits return typical DMG open-bus 0xFF masks.
 */

#include "gb/gb.h"

void apu_init(gb_t *gb)
{
    apu_reset(gb);
}

void apu_reset(gb_t *gb)
{
    (void)memset(gb->apu.nr, 0, sizeof gb->apu.nr);
    gb->apu.enabled = false;
}

void apu_step(gb_t *gb, u32 t_cycles)
{
    (void)gb;
    (void)t_cycles;
}

u8 apu_read(gb_t *gb, u16 addr)
{
    if (addr < 0xFF10 || addr > 0xFF3F) {
        return 0xFF;
    }
    const u16 i = (u16)(addr - 0xFF10);
    static const u8 masks[0x30] = {
        0x80, 0x3F, 0x00, 0xFF, 0xBF, 0xFF, 0x3F, 0x00, 0xFF, 0xBF, 0x7F, 0xFF, 0x9F, 0xFF, 0xBF, 0xFF,
        0xFF, 0x00, 0x00, 0xBF, 0x00, 0x00, 0x70, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    if (!gb->apu.enabled && addr != 0xFF26) {
        return 0xFF;
    }
    return (u8)(gb->apu.nr[i] | masks[i]);
}

void apu_write(gb_t *gb, u16 addr, u8 value)
{
    if (addr < 0xFF10 || addr > 0xFF3F) {
        return;
    }
    if (addr == 0xFF26) {
        gb->apu.enabled = (value & 0x80u) != 0;
        gb->apu.nr[0x16] = (u8)(value & 0x80u);
        if (!gb->apu.enabled) {
            (void)memset(gb->apu.nr, 0, 0x16);
        }
        return;
    }
    if (!gb->apu.enabled && addr < 0xFF30) {
        return;
    }
    gb->apu.nr[addr - 0xFF10] = value;
}
