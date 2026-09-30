/**
 * @file cpu.h
 * @brief Sharp LR35902 CPU state and step API.
 *
 * Hardware reference: Pan Docs § CPU Instruction Set (public hardware notes).
 * Cycle model: T-cycles (1 T-cycle = 1/4194304 s).
 * Thread-safety: not thread-safe; single-threaded core by design.
 * Error handling: illegal opcodes set halted and log via GB_LOG.
 */

#pragma once

#include "gb/types.h"

struct gb;

enum {
    GB_FLAG_Z = 0x80,
    GB_FLAG_N = 0x40,
    GB_FLAG_H = 0x20,
    GB_FLAG_C = 0x10,
    GB_FLAG_UNUSED_MASK = 0x0F /* always written as 0 */
};

typedef struct {
    u8 a;
    u8 f;
    u8 b;
    u8 c;
    u8 d;
    u8 e;
    u8 h;
    u8 l;
    u16 sp;
    u16 pc;
    bool ime;
    bool ime_enable_pending; /**< EI takes effect after the following instruction. */
    bool halted;
    bool halt_bug;           /**< Skip PC increment after halt-when-IME=0 with pending IRQ. */
    bool stopped;
    u8 opcode;               /**< Last fetched opcode (debug). */
    bool cb_prefix;
} cpu_t;

void cpu_init(struct gb *gb);
void cpu_reset(struct gb *gb);

/**
 * @brief Fetch, decode, and execute one instruction (or interrupt service).
 * @return T-cycles consumed (always > 0 unless stopped).
 */
u32 cpu_step(struct gb *gb);

static inline u16 cpu_af(const cpu_t *cpu)
{
    return (u16)(((u16)cpu->a << 8) | (u16)(cpu->f & (u8)~GB_FLAG_UNUSED_MASK));
}

static inline u16 cpu_bc(const cpu_t *cpu)
{
    return (u16)(((u16)cpu->b << 8) | (u16)cpu->c);
}

static inline u16 cpu_de(const cpu_t *cpu)
{
    return (u16)(((u16)cpu->d << 8) | (u16)cpu->e);
}

static inline u16 cpu_hl(const cpu_t *cpu)
{
    return (u16)(((u16)cpu->h << 8) | (u16)cpu->l);
}

static inline void cpu_set_af(cpu_t *cpu, u16 v)
{
    cpu->a = (u8)(v >> 8);
    cpu->f = (u8)(v & (u8)~GB_FLAG_UNUSED_MASK);
}

static inline void cpu_set_bc(cpu_t *cpu, u16 v)
{
    cpu->b = (u8)(v >> 8);
    cpu->c = (u8)(v & 0xFFu);
}

static inline void cpu_set_de(cpu_t *cpu, u16 v)
{
    cpu->d = (u8)(v >> 8);
    cpu->e = (u8)(v & 0xFFu);
}

static inline void cpu_set_hl(cpu_t *cpu, u16 v)
{
    cpu->h = (u8)(v >> 8);
    cpu->l = (u8)(v & 0xFFu);
}

u8 cpu_execute_base(struct gb *gb, u8 opcode);
u8 cpu_execute_cb(struct gb *gb, u8 opcode);
void cpu_set_flags_znhc(cpu_t *cpu, bool z, bool n, bool h, bool c);
bool cpu_flag(const cpu_t *cpu, u8 mask);
