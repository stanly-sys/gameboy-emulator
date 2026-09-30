/**
 * @file cpu_cb.c
 * @brief CB-prefixed rotate/shift/bit ops (256 opcodes).
 *
 * Hardware reference: CB map; (HL) forms cost extra cycles.
 * Cycle model: 8 T-cycles register, 16 T-cycles (HL) [BIT (HL) is 12].
 * Thread-safety: not thread-safe.
 * Error handling: default logs and returns 8.
 */

#include "gb/gb.h"

static u8 *cb_reg(gb_t *gb, u8 idx)
{
    switch (idx) {
    case 0:
        return &gb->cpu.b;
    case 1:
        return &gb->cpu.c;
    case 2:
        return &gb->cpu.d;
    case 3:
        return &gb->cpu.e;
    case 4:
        return &gb->cpu.h;
    case 5:
        return &gb->cpu.l;
    case 7:
        return &gb->cpu.a;
    default:
        return NULL;
    }
}

static u8 cb_get(gb_t *gb, u8 idx)
{
    u8 *p = cb_reg(gb, idx);
    if (p != NULL) {
        return *p;
    }
    return mmu_read(gb, cpu_hl(&gb->cpu));
}

static void cb_set(gb_t *gb, u8 idx, u8 v)
{
    u8 *p = cb_reg(gb, idx);
    if (p != NULL) {
        *p = v;
        return;
    }
    mmu_write(gb, cpu_hl(&gb->cpu), v);
}

u8 cpu_execute_cb(gb_t *gb, u8 opcode)
{
    const u8 y = (u8)((opcode >> 3) & 7u);
    const u8 z = (u8)(opcode & 7u);
    const u8 x = (u8)(opcode >> 6);
    const bool hl = (z == 6);
    u8 val = cb_get(gb, z);
    u8 t = hl ? 16 : 8;

    if (x == 0) {
        u8 res = val;
        bool c = false;
        switch (y) {
        case 0: /* RLC */
            c = (val & 0x80u) != 0;
            res = (u8)((u8)(val << 1) | (c ? 1u : 0u));
            break;
        case 1: /* RRC */
            c = (val & 0x01u) != 0;
            res = (u8)((u8)(val >> 1) | (c ? 0x80u : 0u));
            break;
        case 2: /* RL */
            c = (val & 0x80u) != 0;
            res = (u8)((u8)(val << 1) | (cpu_flag(&gb->cpu, GB_FLAG_C) ? 1u : 0u));
            break;
        case 3: /* RR */
            c = (val & 0x01u) != 0;
            res = (u8)((u8)(val >> 1) | (cpu_flag(&gb->cpu, GB_FLAG_C) ? 0x80u : 0u));
            break;
        case 4: /* SLA */
            c = (val & 0x80u) != 0;
            res = (u8)(val << 1);
            break;
        case 5: /* SRA */
            c = (val & 0x01u) != 0;
            res = (u8)((u8)(val >> 1) | (val & 0x80u));
            break;
        case 6: /* SWAP */
            c = false;
            res = (u8)((u8)(val << 4) | (u8)(val >> 4));
            break;
        case 7: /* SRL */
            c = (val & 0x01u) != 0;
            res = (u8)(val >> 1);
            break;
        default:
            GB_LOG("CB y out of range");
            break;
        }
        cb_set(gb, z, res);
        cpu_set_flags_znhc(&gb->cpu, res == 0, false, false, c);
        return t;
    }
    if (x == 1) {
        /* BIT: (HL) is 12 T-cycles */
        if (hl) {
            t = 12;
        }
        const bool zf = (val & (u8)(1u << y)) == 0;
        cpu_set_flags_znhc(&gb->cpu, zf, false, true, cpu_flag(&gb->cpu, GB_FLAG_C));
        return t;
    }
    if (x == 2) {
        val = (u8)(val & (u8)~(1u << y));
        cb_set(gb, z, val);
        return t;
    }
    if (x == 3) {
        val = (u8)(val | (u8)(1u << y));
        cb_set(gb, z, val);
        return t;
    }
    GB_LOG("illegal CB grouping %02X", opcode);
    gb->cpu.halted = true;
    return 8;
}
