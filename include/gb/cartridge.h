/**
 * @file cartridge.h
 * @brief ROM image, header parse, and save RAM ownership.
 *
 * Hardware reference: cartridge header 0100-014F.
 * Cycle model: none.
 * Thread-safety: not thread-safe.
 * Error handling: untrusted headers; sizes clamped; reject oversized dumps.
 */

#pragma once

#include "gb/types.h"
#include "gb/mbc.h"

struct gb;

enum {
    GB_MAX_ROM_BYTES = 8 * 1024 * 1024,
    GB_MAX_RAM_BYTES = 128 * 1024,
    GB_TITLE_LEN = 16
};

typedef struct {
    u8 *rom;
    size_t rom_size;
    u8 *ram;
    size_t ram_size;
    char title[GB_TITLE_LEN + 1];
    u8 cart_type;
    u8 rom_size_code;
    u8 ram_size_code;
    bool header_ok;
    mbc_t mbc;
} cartridge_t;

void cartridge_init(struct gb *gb);
void cartridge_reset(struct gb *gb);
void cartridge_unload(struct gb *gb);

/**
 * @brief Load an untrusted ROM buffer (copied internally).
 * @return true on success (even if Nintendo logo/checksum warn).
 */
bool cartridge_load(struct gb *gb, const u8 *data, size_t len);

u8 cartridge_read(struct gb *gb, u16 addr);
void cartridge_write(struct gb *gb, u16 addr, u8 value);
