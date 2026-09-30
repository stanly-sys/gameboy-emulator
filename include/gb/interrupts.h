/**
 * @file interrupts.h
 * @brief IF (FF0F) / IE (FFFF) and ISR entry (push PC, clear IME).
 *
 * Hardware reference: VBlank, STAT, Timer, Serial, Joypad priority.
 * Cycle model: ISR costs 20 T-cycles (5 M-cycles).
 * Thread-safety: not thread-safe.
 * Error handling: unknown IF bits preserved as writable.
 */

#pragma once

#include "gb/types.h"

struct gb;

enum {
    GB_INT_VBLANK = 0x01,
    GB_INT_STAT = 0x02,
    GB_INT_TIMER = 0x04,
    GB_INT_SERIAL = 0x08,
    GB_INT_JOYPAD = 0x10
};

typedef struct {
    u8 iff; /**< IF register (lower 5 bits used). */
} interrupts_t;

void interrupts_init(struct gb *gb);
void interrupts_reset(struct gb *gb);
void interrupts_request(struct gb *gb, u8 mask);
u8 interrupts_if_read(const struct gb *gb);
void interrupts_if_write(struct gb *gb, u8 value);

/**
 * @brief Service the highest-priority pending enabled interrupt if IME=1.
 * @return T-cycles if an ISR ran, else 0.
 */
u32 interrupts_service(struct gb *gb);

bool interrupts_pending(const struct gb *gb);
