/**
 * @file joypad.c
 * @brief JOYP matrix: P14 selects D-pad, P15 selects A/B/Select/Start.
 *
 * Hardware reference: pressed buttons drive lines low (0).
 * Cycle model: none.
 * Thread-safety: see joypad.h.
 * Error handling: bits 6-7 read as 1.
 */

#include "gb/gb.h"

void joypad_init(gb_t *gb)
{
    joypad_reset(gb);
}

void joypad_reset(gb_t *gb)
{
    gb->joypad.buttons = 0xFF; /* 1 = released */
    gb->joypad.p1 = 0xCF;
}

void joypad_set_button(gb_t *gb, enum gb_button button, bool pressed)
{
    const u8 mask = (u8)(1u << (unsigned)button);
    const u8 old_nibble = (u8)(joypad_read(gb) & 0x0Fu);
    if (pressed) {
        gb->joypad.buttons = (u8)(gb->joypad.buttons & (u8)~mask);
    } else {
        gb->joypad.buttons = (u8)(gb->joypad.buttons | mask);
    }
    const u8 new_nibble = (u8)(joypad_read(gb) & 0x0Fu);
    /* Falling edge on a selected line requests joypad IRQ. */
    if ((old_nibble & ~new_nibble) != 0) {
        interrupts_request(gb, GB_INT_JOYPAD);
    }
}

u8 joypad_read(gb_t *gb)
{
    u8 result = (u8)((gb->joypad.p1 | 0xCFu) | 0x0Fu);
    const bool select_dpad = (gb->joypad.p1 & 0x10u) == 0;
    const bool select_btns = (gb->joypad.p1 & 0x20u) == 0;
    u8 nibble = 0x0F;
    if (select_dpad) {
        nibble = (u8)(nibble & (gb->joypad.buttons & 0x0Fu));
    }
    if (select_btns) {
        nibble = (u8)(nibble & ((gb->joypad.buttons >> 4) & 0x0Fu));
    }
    result = (u8)((gb->joypad.p1 & 0x30u) | nibble | 0xC0u);
    return result;
}

void joypad_write(gb_t *gb, u8 value)
{
    gb->joypad.p1 = (u8)(value & 0x30u);
}
