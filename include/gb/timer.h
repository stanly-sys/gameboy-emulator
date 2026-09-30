/**
 * @file timer.h
 * @brief DIV/TIMA/TMA/TAC and overflow interrupt 0x50.
 *
 * Hardware reference: FF04-FF07.
 * Cycle model: DIV increments at 16384 Hz (every 256 T-cycles).
 * Thread-safety: not thread-safe.
 * Error handling: TAC unused bits ignored.
 */

#pragma once

#include "gb/types.h"

struct gb;

typedef struct {
    u16 div_counter; /**< Internal 16-bit divider; FF04 is bits 6-13. */
    u8 tima;
    u8 tma;
    u8 tac;
    u16 tima_reload_delay; /**< TIMA=0 for 4 T-cycles after overflow, then TMA. */
} timer_t;

void timer_init(struct gb *gb);
void timer_reset(struct gb *gb);
void timer_step(struct gb *gb, u32 t_cycles);
u8 timer_read(struct gb *gb, u16 addr);
void timer_write(struct gb *gb, u16 addr, u8 value);
