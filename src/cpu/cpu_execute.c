/**
 * @file cpu_execute.c
 * @brief Executes decoded LR35902 opcodes (unprefixed map).
 *
 * Hardware reference: Pan Docs § CPU Instruction Set
 * Cycle model: T-cycles (1 T-cycle = 1/4194304 s)
 * Thread-safety: not thread-safe; single-threaded core by design.
 * Error handling: illegal opcodes set gb->cpu.halted and log via GB_LOG.
 */

#include "gb/gb.h"

static u8 read8(gb_t *gb, u16 a)
{
    return mmu_read(gb, a);
}

static void write8(gb_t *gb, u16 a, u8 v)
{
    mmu_write(gb, a, v);
}

static u8 imm8(gb_t *gb)
{
    const u8 b = read8(gb, gb->cpu.pc);
    gb->cpu.pc = (u16)(gb->cpu.pc + 1u);
    return b;
}

static u16 imm16(gb_t *gb)
{
    const u8 lo = imm8(gb);
    const u8 hi = imm8(gb);
    return (u16)((u16)lo | ((u16)hi << 8));
}

static void push16(gb_t *gb, u16 v)
{
    gb->cpu.sp = (u16)(gb->cpu.sp - 1u);
    write8(gb, gb->cpu.sp, (u8)(v >> 8));
    gb->cpu.sp = (u16)(gb->cpu.sp - 1u);
    write8(gb, gb->cpu.sp, (u8)(v & 0xFFu));
}

static u16 pop16(gb_t *gb)
{
    const u8 lo = read8(gb, gb->cpu.sp);
    gb->cpu.sp = (u16)(gb->cpu.sp + 1u);
    const u8 hi = read8(gb, gb->cpu.sp);
    gb->cpu.sp = (u16)(gb->cpu.sp + 1u);
    return (u16)((u16)lo | ((u16)hi << 8));
}

static u8 *r_tbl(gb_t *gb, u8 i)
{
    switch (i) {
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

static u8 get_r(gb_t *gb, u8 i)
{
    u8 *p = r_tbl(gb, i);
    if (p != NULL) {
        return *p;
    }
    return read8(gb, cpu_hl(&gb->cpu));
}

static void set_r(gb_t *gb, u8 i, u8 v)
{
    u8 *p = r_tbl(gb, i);
    if (p != NULL) {
        *p = v;
        return;
    }
    write8(gb, cpu_hl(&gb->cpu), v);
}

static void add8(gb_t *gb, u8 v, bool carry)
{
    const u16 c = (u16)(carry && cpu_flag(&gb->cpu, GB_FLAG_C) ? 1 : 0);
    const u16 a = gb->cpu.a;
    const u16 s = (u16)(a + v + c);
    const bool h = (((a & 0x0Fu) + (v & 0x0Fu) + c) & 0x10u) != 0;
    cpu_set_flags_znhc(&gb->cpu, (u8)s == 0, false, h, s > 0xFFu);
    gb->cpu.a = (u8)s;
}

static void sub8(gb_t *gb, u8 v, bool carry, bool store)
{
    const u16 c = (u16)(carry && cpu_flag(&gb->cpu, GB_FLAG_C) ? 1 : 0);
    const u16 a = gb->cpu.a;
    const u16 s = (u16)(a - v - c);
    const bool h = ((a & 0x0Fu) < ((v & 0x0Fu) + c));
    cpu_set_flags_znhc(&gb->cpu, (u8)s == 0, true, h, a < (u16)(v + c));
    if (store) {
        gb->cpu.a = (u8)s;
    }
}

static void and8(gb_t *gb, u8 v)
{
    gb->cpu.a = (u8)(gb->cpu.a & v);
    cpu_set_flags_znhc(&gb->cpu, gb->cpu.a == 0, false, true, false);
}

static void xor8(gb_t *gb, u8 v)
{
    gb->cpu.a = (u8)(gb->cpu.a ^ v);
    cpu_set_flags_znhc(&gb->cpu, gb->cpu.a == 0, false, false, false);
}

static void or8(gb_t *gb, u8 v)
{
    gb->cpu.a = (u8)(gb->cpu.a | v);
    cpu_set_flags_znhc(&gb->cpu, gb->cpu.a == 0, false, false, false);
}

static u8 inc8(gb_t *gb, u8 v)
{
    const u8 r = (u8)(v + 1u);
    cpu_set_flags_znhc(&gb->cpu, r == 0, false, (v & 0x0Fu) == 0x0Fu, cpu_flag(&gb->cpu, GB_FLAG_C));
    return r;
}

static u8 dec8(gb_t *gb, u8 v)
{
    const u8 r = (u8)(v - 1u);
    cpu_set_flags_znhc(&gb->cpu, r == 0, true, (v & 0x0Fu) == 0, cpu_flag(&gb->cpu, GB_FLAG_C));
    return r;
}

static void add_hl(gb_t *gb, u16 v)
{
    const u32 hl = cpu_hl(&gb->cpu);
    const u32 s = hl + v;
    const bool h = ((hl & 0x0FFFu) + (v & 0x0FFFu)) > 0x0FFFu;
    cpu_set_flags_znhc(&gb->cpu, cpu_flag(&gb->cpu, GB_FLAG_Z), false, h, s > 0xFFFFu);
    cpu_set_hl(&gb->cpu, (u16)s);
}

static u16 add_sp_e(gb_t *gb, i8 e)
{
    const u16 sp = gb->cpu.sp;
    const u16 r = (u16)(sp + (u16)(i16)e);
    const u16 u = (u16)(u8)e;
    const bool h = ((sp & 0x0Fu) + (u & 0x0Fu)) > 0x0Fu;
    const bool c = ((sp & 0xFFu) + (u & 0xFFu)) > 0xFFu;
    cpu_set_flags_znhc(&gb->cpu, false, false, h, c);
    return r;
}

static void daa(gb_t *gb)
{
    u8 a = gb->cpu.a;
    u8 adj = 0;
    bool c = cpu_flag(&gb->cpu, GB_FLAG_C);
    if (!cpu_flag(&gb->cpu, GB_FLAG_N)) {
        if (c || a > 0x99) {
            adj = (u8)(adj | 0x60u);
            c = true;
        }
        if (cpu_flag(&gb->cpu, GB_FLAG_H) || (a & 0x0Fu) > 9) {
            adj = (u8)(adj | 0x06u);
        }
        a = (u8)(a + adj);
    } else {
        if (c) {
            adj = (u8)(adj | 0x60u);
        }
        if (cpu_flag(&gb->cpu, GB_FLAG_H)) {
            adj = (u8)(adj | 0x06u);
        }
        a = (u8)(a - adj);
    }
    gb->cpu.a = a;
    cpu_set_flags_znhc(&gb->cpu, a == 0, cpu_flag(&gb->cpu, GB_FLAG_N), false, c);
}

u8 cpu_execute_base(gb_t *gb, u8 op)
{
    cpu_t *c = &gb->cpu;
    const u8 x = (u8)(op >> 6);
    const u8 y = (u8)((op >> 3) & 7u);
    const u8 z = (u8)(op & 7u);

    /* 0x00 NOP — no architectural change. */
    if (op == 0x00) {
        return 4;
    }
    /* 0x08 LD (a16), SP */
    if (op == 0x08) {
        const u16 a = imm16(gb);
        write8(gb, a, (u8)(c->sp & 0xFFu));
        write8(gb, (u16)(a + 1u), (u8)(c->sp >> 8));
        return 20;
    }
    /* 0x10 STOP */
    if (op == 0x10) {
        (void)imm8(gb);
        GB_LOG("Game executed STOP at PC=%04X", (unsigned)(c->pc - 2));
        c->stopped = true;
        return 4;
    }
    /* 0x18 JR e */
    if (op == 0x18) {
        const i8 e = (i8)imm8(gb);
        c->pc = (u16)(c->pc + (u16)(i16)e);
        return 12;
    }
    /* 0x27 DAA */
    if (op == 0x27) {
        daa(gb);
        return 4;
    }
    /* 0x2F CPL */
    if (op == 0x2F) {
        c->a = (u8)~c->a;
        cpu_set_flags_znhc(c, cpu_flag(c, GB_FLAG_Z), true, true, cpu_flag(c, GB_FLAG_C));
        return 4;
    }
    /* 0x37 SCF */
    if (op == 0x37) {
        cpu_set_flags_znhc(c, cpu_flag(c, GB_FLAG_Z), false, false, true);
        return 4;
    }
    /* 0x3F CCF */
    if (op == 0x3F) {
        cpu_set_flags_znhc(c, cpu_flag(c, GB_FLAG_Z), false, false, !cpu_flag(c, GB_FLAG_C));
        return 4;
    }
    /* 0x76 HALT */
    if (op == 0x76) {
        if (!c->ime && !c->ime_enable_pending && interrupts_pending(gb)) {
            c->halt_bug = true; /* WHY: HALT bug — next fetch does not raise PC. */
        } else {
            c->halted = true;
        }
        return 4;
    }
    /* 0xC3 JP a16 */
    if (op == 0xC3) {
        c->pc = imm16(gb);
        return 16;
    }
    /* 0xC9 RET */
    if (op == 0xC9) {
        c->pc = pop16(gb);
        return 16;
    }
    /* 0xCD CALL a16 */
    if (op == 0xCD) {
        const u16 a = imm16(gb);
        push16(gb, c->pc);
        c->pc = a;
        return 24;
    }
    /* 0xD9 RETI */
    if (op == 0xD9) {
        c->pc = pop16(gb);
        c->ime = true;
        return 16;
    }
    /* 0xE0 LDH (a8), A */
    if (op == 0xE0) {
        write8(gb, (u16)(0xFF00u + imm8(gb)), c->a);
        return 12;
    }
    /* 0xE2 LD (C), A */
    if (op == 0xE2) {
        write8(gb, (u16)(0xFF00u + c->c), c->a);
        return 8;
    }
    /* 0xE8 ADD SP, e8 */
    if (op == 0xE8) {
        c->sp = add_sp_e(gb, (i8)imm8(gb));
        return 16;
    }
    /* 0xE9 JP HL */
    if (op == 0xE9) {
        c->pc = cpu_hl(c);
        return 4;
    }
    /* 0xEA LD (a16), A */
    if (op == 0xEA) {
        write8(gb, imm16(gb), c->a);
        return 16;
    }
    /* 0xF0 LDH A, (a8) */
    if (op == 0xF0) {
        c->a = read8(gb, (u16)(0xFF00u + imm8(gb)));
        return 12;
    }
    /* 0xF2 LD A, (C) */
    if (op == 0xF2) {
        c->a = read8(gb, (u16)(0xFF00u + c->c));
        return 8;
    }
    /* 0xF3 DI */
    if (op == 0xF3) {
        c->ime = false;
        c->ime_enable_pending = false;
        return 4;
    }
    /* 0xF8 LD HL, SP+e8 */
    if (op == 0xF8) {
        cpu_set_hl(c, add_sp_e(gb, (i8)imm8(gb)));
        return 12;
    }
    /* 0xF9 LD SP, HL */
    if (op == 0xF9) {
        c->sp = cpu_hl(c);
        return 8;
    }
    /* 0xFA LD A, (a16) */
    if (op == 0xFA) {
        c->a = read8(gb, imm16(gb));
        return 16;
    }
    /* 0xFB EI */
    if (op == 0xFB) {
        c->ime_enable_pending = true; /* WHY: IME rises after the next instruction. */
        return 4;
    }

    /* LD r16, d16  / INC r16 / DEC r16 / ADD HL, r16  (column z=1,3 and 0x09 family) */
    if ((op & 0xCFu) == 0x01) {
        /* 0x01,11,21,31 LD rr,d16 */
        const u16 v = imm16(gb);
        switch ((op >> 4) & 3u) {
        case 0:
            cpu_set_bc(c, v);
            break;
        case 1:
            cpu_set_de(c, v);
            break;
        case 2:
            cpu_set_hl(c, v);
            break;
        default:
            c->sp = v;
            break;
        }
        return 12;
    }
    if ((op & 0xCFu) == 0x03) {
        /* INC rr */
        switch ((op >> 4) & 3u) {
        case 0:
            cpu_set_bc(c, (u16)(cpu_bc(c) + 1u));
            break;
        case 1:
            cpu_set_de(c, (u16)(cpu_de(c) + 1u));
            break;
        case 2:
            cpu_set_hl(c, (u16)(cpu_hl(c) + 1u));
            break;
        default:
            c->sp = (u16)(c->sp + 1u);
            break;
        }
        return 8;
    }
    if ((op & 0xCFu) == 0x0B) {
        switch ((op >> 4) & 3u) {
        case 0:
            cpu_set_bc(c, (u16)(cpu_bc(c) - 1u));
            break;
        case 1:
            cpu_set_de(c, (u16)(cpu_de(c) - 1u));
            break;
        case 2:
            cpu_set_hl(c, (u16)(cpu_hl(c) - 1u));
            break;
        default:
            c->sp = (u16)(c->sp - 1u);
            break;
        }
        return 8;
    }
    if ((op & 0xCFu) == 0x09) {
        u16 v = 0;
        switch ((op >> 4) & 3u) {
        case 0:
            v = cpu_bc(c);
            break;
        case 1:
            v = cpu_de(c);
            break;
        case 2:
            v = cpu_hl(c);
            break;
        default:
            v = c->sp;
            break;
        }
        add_hl(gb, v);
        return 8;
    }

    /* LD (rr), A / LD A, (rr) / LDI / LDD */
    if (op == 0x02) {
        write8(gb, cpu_bc(c), c->a);
        return 8;
    }
    if (op == 0x12) {
        write8(gb, cpu_de(c), c->a);
        return 8;
    }
    if (op == 0x22) {
        write8(gb, cpu_hl(c), c->a);
        cpu_set_hl(c, (u16)(cpu_hl(c) + 1u));
        return 8;
    }
    if (op == 0x32) {
        write8(gb, cpu_hl(c), c->a);
        cpu_set_hl(c, (u16)(cpu_hl(c) - 1u));
        return 8;
    }
    if (op == 0x0A) {
        c->a = read8(gb, cpu_bc(c));
        return 8;
    }
    if (op == 0x1A) {
        c->a = read8(gb, cpu_de(c));
        return 8;
    }
    if (op == 0x2A) {
        c->a = read8(gb, cpu_hl(c));
        cpu_set_hl(c, (u16)(cpu_hl(c) + 1u));
        return 8;
    }
    if (op == 0x3A) {
        c->a = read8(gb, cpu_hl(c));
        cpu_set_hl(c, (u16)(cpu_hl(c) - 1u));
        return 8;
    }

    /* INC/DEC r8 and LD r8,d8 */
    if ((op & 0xC7u) == 0x04) {
        const u8 v = inc8(gb, get_r(gb, y));
        set_r(gb, y, v);
        return (y == 6) ? 12 : 4;
    }
    if ((op & 0xC7u) == 0x05) {
        const u8 v = dec8(gb, get_r(gb, y));
        set_r(gb, y, v);
        return (y == 6) ? 12 : 4;
    }
    if ((op & 0xC7u) == 0x06) {
        set_r(gb, y, imm8(gb));
        return (y == 6) ? 12 : 8;
    }

    /* RLCA RRCA RLA RRA */
    if (op == 0x07 || op == 0x0F || op == 0x17 || op == 0x1F) {
        u8 a = c->a;
        bool cy;
        if (op == 0x07) {
            cy = (a & 0x80u) != 0;
            a = (u8)((u8)(a << 1) | (cy ? 1u : 0u));
        } else if (op == 0x0F) {
            cy = (a & 1u) != 0;
            a = (u8)((u8)(a >> 1) | (cy ? 0x80u : 0u));
        } else if (op == 0x17) {
            cy = (a & 0x80u) != 0;
            a = (u8)((u8)(a << 1) | (cpu_flag(c, GB_FLAG_C) ? 1u : 0u));
        } else {
            cy = (a & 1u) != 0;
            a = (u8)((u8)(a >> 1) | (cpu_flag(c, GB_FLAG_C) ? 0x80u : 0u));
        }
        c->a = a;
        cpu_set_flags_znhc(c, false, false, false, cy);
        return 4;
    }

    /* JR cc, e */
    if (op == 0x20 || op == 0x28 || op == 0x30 || op == 0x38) {
        const i8 e = (i8)imm8(gb);
        bool take = false;
        switch (op) {
        case 0x20:
            take = !cpu_flag(c, GB_FLAG_Z);
            break;
        case 0x28:
            take = cpu_flag(c, GB_FLAG_Z);
            break;
        case 0x30:
            take = !cpu_flag(c, GB_FLAG_C);
            break;
        default:
            take = cpu_flag(c, GB_FLAG_C);
            break;
        }
        if (take) {
            c->pc = (u16)(c->pc + (u16)(i16)e);
            return 12;
        }
        return 8;
    }

    /* LD r, r'  (0x40-0x7F except HALT) */
    if (x == 1) {
        set_r(gb, y, get_r(gb, z));
        return (y == 6 || z == 6) ? 8 : 4;
    }

    /* ALU A, r  (0x80-0xBF) */
    if (x == 2) {
        const u8 v = get_r(gb, z);
        switch (y) {
        case 0:
            add8(gb, v, false);
            break;
        case 1:
            add8(gb, v, true);
            break;
        case 2:
            sub8(gb, v, false, true);
            break;
        case 3:
            sub8(gb, v, true, true);
            break;
        case 4:
            and8(gb, v);
            break;
        case 5:
            xor8(gb, v);
            break;
        case 6:
            or8(gb, v);
            break;
        default:
            sub8(gb, v, false, false);
            break;
        }
        return (z == 6) ? 8 : 4;
    }

    /* ALU A, d8 */
    if (op == 0xC6) {
        add8(gb, imm8(gb), false);
        return 8;
    }
    if (op == 0xCE) {
        add8(gb, imm8(gb), true);
        return 8;
    }
    if (op == 0xD6) {
        sub8(gb, imm8(gb), false, true);
        return 8;
    }
    if (op == 0xDE) {
        sub8(gb, imm8(gb), true, true);
        return 8;
    }
    if (op == 0xE6) {
        and8(gb, imm8(gb));
        return 8;
    }
    if (op == 0xEE) {
        xor8(gb, imm8(gb));
        return 8;
    }
    if (op == 0xF6) {
        or8(gb, imm8(gb));
        return 8;
    }
    if (op == 0xFE) {
        sub8(gb, imm8(gb), false, false);
        return 8;
    }

    /* RET cc / JP cc / CALL cc */
    if ((op & 0xE7u) == 0xC0) {
        bool take = false;
        switch ((op >> 3) & 3u) {
        case 0:
            take = !cpu_flag(c, GB_FLAG_Z);
            break;
        case 1:
            take = cpu_flag(c, GB_FLAG_Z);
            break;
        case 2:
            take = !cpu_flag(c, GB_FLAG_C);
            break;
        default:
            take = cpu_flag(c, GB_FLAG_C);
            break;
        }
        if ((op & 0x07u) == 0x00) {
            if (take) {
                c->pc = pop16(gb);
                return 20;
            }
            return 8;
        }
    }
    if ((op & 0xE7u) == 0xC2) {
        const u16 a = imm16(gb);
        bool take = false;
        switch ((op >> 3) & 3u) {
        case 0:
            take = !cpu_flag(c, GB_FLAG_Z);
            break;
        case 1:
            take = cpu_flag(c, GB_FLAG_Z);
            break;
        case 2:
            take = !cpu_flag(c, GB_FLAG_C);
            break;
        default:
            take = cpu_flag(c, GB_FLAG_C);
            break;
        }
        if (take) {
            c->pc = a;
            return 16;
        }
        return 12;
    }
    if ((op & 0xE7u) == 0xC4) {
        const u16 a = imm16(gb);
        bool take = false;
        switch ((op >> 3) & 3u) {
        case 0:
            take = !cpu_flag(c, GB_FLAG_Z);
            break;
        case 1:
            take = cpu_flag(c, GB_FLAG_Z);
            break;
        case 2:
            take = !cpu_flag(c, GB_FLAG_C);
            break;
        default:
            take = cpu_flag(c, GB_FLAG_C);
            break;
        }
        if (take) {
            push16(gb, c->pc);
            c->pc = a;
            return 24;
        }
        return 12;
    }

    /* POP/PUSH rr */
    if ((op & 0xCFu) == 0xC1) {
        const u16 v = pop16(gb);
        switch ((op >> 4) & 3u) {
        case 0:
            cpu_set_bc(c, v);
            break;
        case 1:
            cpu_set_de(c, v);
            break;
        case 2:
            cpu_set_hl(c, v);
            break;
        default:
            cpu_set_af(c, v);
            break;
        }
        return 12;
    }
    if ((op & 0xCFu) == 0xC5) {
        u16 v = 0;
        switch ((op >> 4) & 3u) {
        case 0:
            v = cpu_bc(c);
            break;
        case 1:
            v = cpu_de(c);
            break;
        case 2:
            v = cpu_hl(c);
            break;
        default:
            v = cpu_af(c);
            break;
        }
        push16(gb, v);
        return 16;
    }

    /* RST */
    if ((op & 0xC7u) == 0xC7) {
        push16(gb, c->pc);
        c->pc = (u16)(op & 0x38u);
        return 16;
    }

    /* Unused opcodes on LR35902: D3, DB, DD, E3, E4, EB, EC, ED, F4, FC, FD */
    GB_LOG("illegal opcode %02X at PC=%04X — halting", op, (unsigned)(c->pc - 1u));
    c->halted = true;
    return 4;
}
