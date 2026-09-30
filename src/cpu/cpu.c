/**
 * @file cpu.c
 * @brief CPU init/reset/step and gb_t frame driver.
 *
 * Hardware reference: post-boot DMG register values when no boot ROM.
 * Cycle model: cpu_step returns T-cycles; gb_run_frame budgets 70224.
 * Thread-safety: not thread-safe.
 * Error handling: STOP waits until a button press.
 */

#include "gb/gb.h"

#include <stdlib.h>

void cpu_init(gb_t *gb)
{
    cpu_reset(gb);
}

void cpu_reset(gb_t *gb)
{
    cpu_t *c = &gb->cpu;
    if (gb->mmu.boot_rom_enabled) {
        c->a = 0;
        c->f = 0;
        c->b = 0;
        c->c = 0;
        c->d = 0;
        c->e = 0;
        c->h = 0;
        c->l = 0;
        c->sp = 0;
        c->pc = 0;
    } else {
        /* WHY: skip boot ROM; match DMG after logo checksum. */
        c->a = 0x01;
        c->f = 0xB0;
        c->b = 0x00;
        c->c = 0x13;
        c->d = 0x00;
        c->e = 0xD8;
        c->h = 0x01;
        c->l = 0x4D;
        c->sp = 0xFFFE;
        c->pc = 0x0100;
    }
    c->ime = false;
    c->ime_enable_pending = false;
    c->halted = false;
    c->halt_bug = false;
    c->stopped = false;
    c->cb_prefix = false;
}

static u8 fetch8(gb_t *gb)
{
    const u8 b = mmu_read(gb, gb->cpu.pc);
    if (gb->cpu.halt_bug) {
        gb->cpu.halt_bug = false;
        return b;
    }
    gb->cpu.pc = (u16)(gb->cpu.pc + 1u);
    return b;
}

u32 cpu_step(gb_t *gb)
{
    if (gb->cpu.stopped) {
        if ((gb->joypad.buttons & 0xFFu) != 0xFFu) {
            gb->cpu.stopped = false;
        } else {
            return 4;
        }
    }

    if (gb->cpu.halted) {
        if (interrupts_pending(gb)) {
            gb->cpu.halted = false;
        } else {
            return 4;
        }
    }

    {
        const u32 irq_t = interrupts_service(gb);
        if (irq_t != 0) {
            return irq_t;
        }
    }

    const bool ei_was_pending = gb->cpu.ime_enable_pending;
    const u8 op = fetch8(gb);
    gb->cpu.opcode = op;
    u8 t;
    if (op == 0xCB) {
        const u8 cb = fetch8(gb);
        t = cpu_execute_cb(gb, cb);
    } else {
        t = cpu_execute_base(gb, op);
    }
    if (ei_was_pending) {
        gb->cpu.ime = true;
        gb->cpu.ime_enable_pending = false;
    }
    debugger_on_instruction(gb);
    return t == 0 ? 4 : t;
}

void gb_init(gb_t *gb)
{
    (void)memset(gb, 0, sizeof *gb);
    cartridge_init(gb);
    mmu_init(gb);
    cpu_init(gb);
    ppu_init(gb);
    apu_init(gb);
    timer_init(gb);
    joypad_init(gb);
    interrupts_init(gb);
    debugger_init(gb);
    gb->running = true;
}

void gb_reset(gb_t *gb)
{
    mmu_reset(gb);
    cpu_reset(gb);
    ppu_reset(gb);
    apu_reset(gb);
    timer_reset(gb);
    joypad_reset(gb);
    interrupts_reset(gb);
    gb->cycles = 0;
    gb->serial_len = 0;
    gb->serial_out[0] = '\0';
}

void gb_destroy(gb_t *gb)
{
    cartridge_unload(gb);
}

bool gb_load_rom_bytes(gb_t *gb, const u8 *data, size_t len)
{
    if (!cartridge_load(gb, data, len)) {
        return false;
    }
    gb_reset(gb);
    return true;
}

bool gb_load_rom_file(gb_t *gb, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        GB_LOG("cannot open ROM %s", path);
        return false;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        (void)fclose(f);
        return false;
    }
    const long sz = ftell(f);
    if (sz <= 0 || (size_t)sz > GB_MAX_ROM_BYTES) {
        GB_LOG("ROM size rejected (%ld)", sz);
        (void)fclose(f);
        return false;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        (void)fclose(f);
        return false;
    }
    u8 *buf = (u8 *)malloc((size_t)sz);
    if (buf == NULL) {
        (void)fclose(f);
        return false;
    }
    const size_t n = fread(buf, 1, (size_t)sz, f);
    (void)fclose(f);
    const bool ok = (n == (size_t)sz) && gb_load_rom_bytes(gb, buf, n);
    free(buf);
    return ok;
}

u32 gb_step(gb_t *gb)
{
    const u32 t = cpu_step(gb);
    ppu_step(gb, t);
    timer_step(gb, t);
    apu_step(gb, t);
    dma_step(gb, t);
    gb->cycles += t;
    return t;
}

void gb_run_frame(gb_t *gb)
{
    const u64 start = gb->cycles;
    while ((gb->cycles - start) < (u64)GB_DOTS_PER_FRAME) {
        if (gb->paused || (gb->dbg.enabled && gb->dbg.break_next)) {
            break;
        }
        (void)gb_step(gb);
    }
}

void gb_set_button(gb_t *gb, enum gb_button button, bool pressed)
{
    joypad_set_button(gb, button, pressed);
}

const u8 *gb_framebuffer(const gb_t *gb)
{
    return gb->framebuffer;
}
