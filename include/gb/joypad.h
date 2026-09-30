/**
 * @file joypad.h
 * @brief P1/JOYP (FF00) button matrix.
 *
 * Hardware reference: P10-P13 nibble, P14/P15 select.
 * Cycle model: polling only; interrupt on falling edge of buttons.
 * Thread-safety: host may call gb_set_button from the UI thread if it
 *                synchronizes with the emulation loop (not internally locked).
 * Error handling: unused bits read as 1.
 */

#pragma once

#include "gb/types.h"

struct gb;

enum gb_button {
    GB_BTN_RIGHT = 0,
    GB_BTN_LEFT = 1,
    GB_BTN_UP = 2,
    GB_BTN_DOWN = 3,
    GB_BTN_A = 4,
    GB_BTN_B = 5,
    GB_BTN_SELECT = 6,
    GB_BTN_START = 7
};

typedef struct {
    u8 buttons; /**< 0 = pressed (hardware inverted). bits 0-7 as gb_button. */
    u8 p1;      /**< Written select bits in 4-5. */
} joypad_t;

void joypad_init(struct gb *gb);
void joypad_reset(struct gb *gb);
u8 joypad_read(struct gb *gb);
void joypad_write(struct gb *gb, u8 value);
void joypad_set_button(struct gb *gb, enum gb_button button, bool pressed);
