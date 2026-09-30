/**
 * @file gb.h
 * @brief Public umbrella: owned core state and host-facing lifecycle API.
 *
 * Hardware reference: Nintendo DMG-01 as a single clocked SoC.
 * Cycle model: host calls gb_run_frame (70224 T-cycles) or gb_step.
 * Thread-safety: not thread-safe; one gb_t per thread.
 * Error handling: failed ROM load leaves previous cart unloaded.
 */

#pragma once

#include "gb/types.h"
#include "gb/cpu.h"
#include "gb/mmu.h"
#include "gb/ppu.h"
#include "gb/apu.h"
#include "gb/timer.h"
#include "gb/joypad.h"
#include "gb/cartridge.h"
#include "gb/mbc.h"
#include "gb/interrupts.h"
#include "gb/debugger.h"

#include <stdio.h>

#ifndef GB_LOG
#define GB_LOG(...)                                                                                  \
    do {                                                                                             \
        (void)fprintf(stderr, "[gb] ");                                                              \
        (void)fprintf(stderr, __VA_ARGS__);                                                          \
        (void)fprintf(stderr, "\n");                                                                 \
    } while (0)
#endif

typedef struct gb {
    cpu_t cpu;
    mmu_t mmu;
    ppu_t ppu;
    apu_t apu;
    timer_t timer;
    joypad_t joypad;
    cartridge_t cart;
    interrupts_t irq;
    u8 framebuffer[160 * 144 * 4]; /**< RGBA8, row-major. */
    u64 cycles;                    /**< Total elapsed T-cycles. */
    bool running;
    bool paused;
    debugger_t dbg;
    u8 serial_data; /**< SB (FF01) for Blargg-style tests. */
    u8 serial_control;
    char serial_out[4096];
    size_t serial_len;
} gb_t;

void gb_init(gb_t *gb);
void gb_reset(gb_t *gb);
void gb_destroy(gb_t *gb);

bool gb_load_rom_file(gb_t *gb, const char *path);
bool gb_load_rom_bytes(gb_t *gb, const u8 *data, size_t len);

u32 gb_step(gb_t *gb);
void gb_run_frame(gb_t *gb);

void gb_set_button(gb_t *gb, enum gb_button button, bool pressed);

const u8 *gb_framebuffer(const gb_t *gb);
