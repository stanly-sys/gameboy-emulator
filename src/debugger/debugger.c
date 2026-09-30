/**
 * @file debugger.c
 * @brief Breakpoints, watches, and versioned savestate blobs.
 *
 * Savestate is a dump of gb_t with ROM/RAM pointers rewritten on load.
 */

#include "gb/gb.h"

#include <stdlib.h>

void debugger_init(gb_t *gb)
{
    debugger_reset(gb);
}

void debugger_reset(gb_t *gb)
{
    (void)memset(&gb->dbg, 0, sizeof gb->dbg);
}

void debugger_on_instruction(gb_t *gb)
{
    if (!gb->dbg.enabled) {
        return;
    }
    u8 i;
    for (i = 0; i < gb->dbg.breakpoint_count; i++) {
        if (gb->cpu.pc == gb->dbg.breakpoints[i]) {
            gb->dbg.break_next = true;
            gb->paused = true;
        }
    }
    if (gb->dbg.step_over && gb->cpu.pc == gb->dbg.step_over_pc) {
        gb->dbg.step_over = false;
        gb->dbg.break_next = true;
        gb->paused = true;
    }
}

bool debugger_add_breakpoint(gb_t *gb, u16 pc)
{
    if (gb->dbg.breakpoint_count >= GB_DBG_MAX_BP) {
        return false;
    }
    gb->dbg.breakpoints[gb->dbg.breakpoint_count++] = pc;
    return true;
}

bool debugger_add_watch(gb_t *gb, u16 addr)
{
    if (gb->dbg.watch_count >= GB_DBG_MAX_WATCH) {
        return false;
    }
    gb->dbg.watches[gb->dbg.watch_count++] = addr;
    return true;
}

bool gb_save_state(gb_t *gb, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        return false;
    }
    u8 hdr[16];
    gb_store_le32(hdr, GB_STATE_MAGIC);
    gb_store_le32(hdr + 4, GB_STATE_VERSION);
    gb_store_le32(hdr + 8, (u32)gb->cart.rom_size);
    gb_store_le32(hdr + 12, (u32)gb->cart.ram_size);
    if (fwrite(hdr, 1, 16, f) != 16) {
        (void)fclose(f);
        return false;
    }
    /* Snapshot CPU/PPU/timer/IRQ/joypad/framebuffer/cycles — not heap pointers. */
    if (fwrite(&gb->cpu, 1, sizeof gb->cpu, f) != sizeof gb->cpu) {
        (void)fclose(f);
        return false;
    }
    if (fwrite(&gb->mmu, 1, sizeof gb->mmu, f) != sizeof gb->mmu) {
        (void)fclose(f);
        return false;
    }
    if (fwrite(&gb->ppu, 1, sizeof gb->ppu, f) != sizeof gb->ppu) {
        (void)fclose(f);
        return false;
    }
    if (fwrite(&gb->timer, 1, sizeof gb->timer, f) != sizeof gb->timer) {
        (void)fclose(f);
        return false;
    }
    if (fwrite(&gb->joypad, 1, sizeof gb->joypad, f) != sizeof gb->joypad) {
        (void)fclose(f);
        return false;
    }
    if (fwrite(&gb->irq, 1, sizeof gb->irq, f) != sizeof gb->irq) {
        (void)fclose(f);
        return false;
    }
    if (fwrite(&gb->cart.mbc, 1, sizeof gb->cart.mbc, f) != sizeof gb->cart.mbc) {
        (void)fclose(f);
        return false;
    }
    if (fwrite(gb->framebuffer, 1, sizeof gb->framebuffer, f) != sizeof gb->framebuffer) {
        (void)fclose(f);
        return false;
    }
    if (fwrite(&gb->cycles, 1, sizeof gb->cycles, f) != sizeof gb->cycles) {
        (void)fclose(f);
        return false;
    }
    if (gb->cart.ram != NULL && gb->cart.ram_size > 0) {
        if (fwrite(gb->cart.ram, 1, gb->cart.ram_size, f) != gb->cart.ram_size) {
            (void)fclose(f);
            return false;
        }
    }
    (void)fclose(f);
    return true;
}

bool gb_load_state(gb_t *gb, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return false;
    }
    u8 hdr[16];
    if (fread(hdr, 1, 16, f) != 16) {
        (void)fclose(f);
        return false;
    }
    if (gb_load_le32(hdr) != GB_STATE_MAGIC || gb_load_le32(hdr + 4) != GB_STATE_VERSION) {
        GB_LOG("savestate version mismatch");
        (void)fclose(f);
        return false;
    }
    if (gb_load_le32(hdr + 8) != (u32)gb->cart.rom_size ||
        gb_load_le32(hdr + 12) != (u32)gb->cart.ram_size) {
        GB_LOG("savestate ROM/RAM size mismatch");
        (void)fclose(f);
        return false;
    }
    if (fread(&gb->cpu, 1, sizeof gb->cpu, f) != sizeof gb->cpu) {
        (void)fclose(f);
        return false;
    }
    if (fread(&gb->mmu, 1, sizeof gb->mmu, f) != sizeof gb->mmu) {
        (void)fclose(f);
        return false;
    }
    if (fread(&gb->ppu, 1, sizeof gb->ppu, f) != sizeof gb->ppu) {
        (void)fclose(f);
        return false;
    }
    if (fread(&gb->timer, 1, sizeof gb->timer, f) != sizeof gb->timer) {
        (void)fclose(f);
        return false;
    }
    if (fread(&gb->joypad, 1, sizeof gb->joypad, f) != sizeof gb->joypad) {
        (void)fclose(f);
        return false;
    }
    if (fread(&gb->irq, 1, sizeof gb->irq, f) != sizeof gb->irq) {
        (void)fclose(f);
        return false;
    }
    if (fread(&gb->cart.mbc, 1, sizeof gb->cart.mbc, f) != sizeof gb->cart.mbc) {
        (void)fclose(f);
        return false;
    }
    if (fread(gb->framebuffer, 1, sizeof gb->framebuffer, f) != sizeof gb->framebuffer) {
        (void)fclose(f);
        return false;
    }
    if (fread(&gb->cycles, 1, sizeof gb->cycles, f) != sizeof gb->cycles) {
        (void)fclose(f);
        return false;
    }
    if (gb->cart.ram != NULL && gb->cart.ram_size > 0) {
        if (fread(gb->cart.ram, 1, gb->cart.ram_size, f) != gb->cart.ram_size) {
            (void)fclose(f);
            return false;
        }
    }
    (void)fclose(f);
    return true;
}
