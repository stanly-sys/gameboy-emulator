/**
 * @file debugger.h
 * @brief Desktop debugger: breakpoints, watchpoints, disassembly, savestate.
 *
 * Hardware reference: none (host tooling).
 * Cycle model: none.
 * Thread-safety: not thread-safe.
 * Error handling: savestate version mismatch refuses to load.
 */

#pragma once

#include "gb/types.h"

struct gb;

enum {
    GB_DBG_MAX_BP = 64,
    GB_DBG_MAX_WATCH = 32,
    GB_STATE_MAGIC = 0x31424747u, /* 'GBG1' */
    GB_STATE_VERSION = 1u
};

typedef struct {
    bool enabled;
    bool break_next;
    bool step_over;
    u16 step_over_pc;
    u16 breakpoints[GB_DBG_MAX_BP];
    u8 breakpoint_count;
    u16 watches[GB_DBG_MAX_WATCH];
    u8 watch_count;
} debugger_t;

void debugger_init(struct gb *gb);
void debugger_reset(struct gb *gb);
void debugger_on_instruction(struct gb *gb);
bool debugger_add_breakpoint(struct gb *gb, u16 pc);
bool debugger_add_watch(struct gb *gb, u16 addr);
int disassemble_at(struct gb *gb, u16 addr, char *out, size_t out_cap);
bool gb_save_state(struct gb *gb, const char *path);
bool gb_load_state(struct gb *gb, const char *path);
