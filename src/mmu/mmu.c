/**
 * @file mmu.c
 * @brief CPU-visible 64 KiB map. Cartridge/PPU/timer side effects live in helpers.
 *
 * Hardware reference: Nintendo forbidden echo RAM still mirrors WRAM here.
 * Cycle model: none (wait-states not modeled).
 * Thread-safety: not thread-safe.
 * Error handling: ROM writes logged; unmapped = 0xFF.
 */

#include "gb/gb.h"

void mmu_init(gb_t *gb)
{
    mmu_reset(gb);
}

void mmu_reset(gb_t *gb)
{
    (void)memset(gb->mmu.vram, 0, sizeof gb->mmu.vram);
    (void)memset(gb->mmu.wram, 0, sizeof gb->mmu.wram);
    (void)memset(gb->mmu.oam, 0, sizeof gb->mmu.oam);
    (void)memset(gb->mmu.hram, 0, sizeof gb->mmu.hram);
    (void)memset(gb->mmu.io, 0xFF, sizeof gb->mmu.io);
    gb->mmu.ie = 0;
    gb->mmu.dma_bytes_left = 0;
    gb->mmu.boot_rom_enabled = gb->mmu.boot_rom_present;
}

bool mmu_load_boot_rom(gb_t *gb, const u8 *data, size_t len)
{
    if (data == NULL || len != GB_BOOT_ROM_SIZE) {
        return false;
    }
    (void)gb_memcpy_bounded(gb->mmu.boot_rom, sizeof gb->mmu.boot_rom, data, len);
    gb->mmu.boot_rom_present = true;
    gb->mmu.boot_rom_enabled = true;
    return true;
}

u8 mmu_read(gb_t *gb, u16 addr)
{
    if (gb->mmu.boot_rom_enabled && addr < GB_BOOT_ROM_SIZE) {
        return gb->mmu.boot_rom[addr];
    }
    if (addr < 0x8000) {
        return cartridge_read(gb, addr);
    }
    if (addr < 0xA000) {
        return gb->mmu.vram[addr - 0x8000];
    }
    if (addr < 0xC000) {
        return cartridge_read(gb, addr);
    }
    if (addr < 0xE000) {
        return gb->mmu.wram[addr - 0xC000];
    }
    if (addr < 0xFE00) {
        return gb->mmu.wram[addr - 0xE000]; /* Echo RAM */
    }
    if (addr < 0xFEA0) {
        return gb->mmu.oam[addr - 0xFE00];
    }
    if (addr < 0xFF00) {
        return 0xFF; /* Unusable */
    }
    if (addr < 0xFF80) {
        return mmu_read_io(gb, addr);
    }
    if (addr < 0xFFFF) {
        return gb->mmu.hram[addr - 0xFF80];
    }
    return gb->mmu.ie;
}

void mmu_write(gb_t *gb, u16 addr, u8 value)
{
    if (addr < 0x8000) {
        cartridge_write(gb, addr, value);
        return;
    }
    if (addr < 0xA000) {
        gb->mmu.vram[addr - 0x8000] = value;
        return;
    }
    if (addr < 0xC000) {
        cartridge_write(gb, addr, value);
        return;
    }
    if (addr < 0xE000) {
        gb->mmu.wram[addr - 0xC000] = value;
        return;
    }
    if (addr < 0xFE00) {
        gb->mmu.wram[addr - 0xE000] = value;
        return;
    }
    if (addr < 0xFEA0) {
        gb->mmu.oam[addr - 0xFE00] = value;
        return;
    }
    if (addr < 0xFF00) {
        return;
    }
    if (addr < 0xFF80) {
        mmu_write_io(gb, addr, value);
        return;
    }
    if (addr < 0xFFFF) {
        gb->mmu.hram[addr - 0xFF80] = value;
        return;
    }
    gb->mmu.ie = value;
}
