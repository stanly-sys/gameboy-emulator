/**
 * @file apu.h
 * @brief Placeholder APU: register file only until sound is implemented.
 *
 * Hardware reference: NR10-NR52, wave RAM FF30-FF3F.
 * Cycle model: not stepped yet (returns immediately).
 * Thread-safety: not thread-safe.
 * Error handling: writes accepted; audio output is silence.
 */

#pragma once

#include "gb/types.h"

struct gb;

typedef struct {
    u8 nr[0x30]; /**< FF10-FF3F image. */
    bool enabled;
} apu_t;

void apu_init(struct gb *gb);
void apu_reset(struct gb *gb);
void apu_step(struct gb *gb, u32 t_cycles);
u8 apu_read(struct gb *gb, u16 addr);
void apu_write(struct gb *gb, u16 addr, u8 value);
