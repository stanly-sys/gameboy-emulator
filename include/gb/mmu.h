/**
 * @file mmu.h
 * @brief 16-bit address space: ROM, VRAM, WRAM, OAM, I/O, HRAM.
 *
 * Hardware reference: DMG memory map 0000-FFFF.
 * Cycle model: DMA burns 160 T-cycles of bus lock (approximated).
 * Thread-safety: not thread-safe.
 * Error handling: unmapped reads return 0xFF; ROM writes ignored + logged.
 */

#pragma once

#include "gb/types.h"

struct gb;

enum {
    GB_BOOT_ROM_SIZE = 256,
    GB_VRAM_SIZE = 0x2000,
    GB_WRAM_SIZE = 0x2000,
    GB_OAM_SIZE = 0xA0,
    GB_HRAM_SIZE = 0x7F,
    GB_IO_SIZE = 0x80
};

typedef struct {
    u8 boot_rom[GB_BOOT_ROM_SIZE];
    bool boot_rom_enabled; /**< Cleared when FF50 is written with bit0 set. */
    bool boot_rom_present;
    u8 vram[GB_VRAM_SIZE];
    u8 wram[GB_WRAM_SIZE];
    u8 oam[GB_OAM_SIZE];
    u8 hram[GB_HRAM_SIZE];
    u8 io[GB_IO_SIZE];
    u8 ie; /**< FFFF interrupt enable. */
    u16 dma_src;
    u16 dma_bytes_left; /**< Remaining OAM DMA copies; 0 = idle. */
} mmu_t;

void mmu_init(struct gb *gb);
void mmu_reset(struct gb *gb);

u8 mmu_read(struct gb *gb, u16 addr);
void mmu_write(struct gb *gb, u16 addr, u8 value);

u8 mmu_read_io(struct gb *gb, u16 addr);
void mmu_write_io(struct gb *gb, u16 addr, u8 value);

void dma_start(struct gb *gb, u8 page);
void dma_step(struct gb *gb, u32 t_cycles);

/** @brief Optional 256-byte DMG boot ROM; safe to skip (PC starts at 0x0100). */
bool mmu_load_boot_rom(struct gb *gb, const u8 *data, size_t len);
