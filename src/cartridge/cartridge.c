/**
 * @file cartridge.c
 * @brief Header parse, allocation, and MBC kind selection.
 *
 * Hardware reference: 0147 type, 0148 ROM size, 0149 RAM size.
 * Cycle model: none.
 * Thread-safety: not thread-safe.
 * Error handling: never trust header; clamp to file size; reject >8MiB ROM.
 */

#include "gb/gb.h"

#include <stdlib.h>

void cartridge_init(gb_t *gb)
{
    (void)memset(&gb->cart, 0, sizeof gb->cart);
}

void cartridge_unload(gb_t *gb)
{
    free(gb->cart.rom);
    free(gb->cart.ram);
    gb->cart.rom = NULL;
    gb->cart.ram = NULL;
    gb->cart.rom_size = 0;
    gb->cart.ram_size = 0;
}

void cartridge_reset(gb_t *gb)
{
    (void)gb;
}

static size_t rom_bytes_from_code(u8 code, size_t file_len)
{
    size_t n = (32u * 1024u) << (code > 8 ? 8 : code);
    if (n > file_len) {
        n = file_len;
    }
    if (n > GB_MAX_ROM_BYTES) {
        n = GB_MAX_ROM_BYTES;
    }
    /* WHY: round up to 16 KiB banks so bank math never reads past allocation. */
    if (n < 0x8000) {
        n = 0x8000;
    }
    return n;
}

static size_t ram_bytes_from_code(u8 code, gb_mbc_kind kind)
{
    if (kind == GB_MBC_MBC2) {
        return 512;
    }
    switch (code) {
    case 0:
        return 0;
    case 1:
        return 2 * 1024;
    case 2:
        return 8 * 1024;
    case 3:
        return 32 * 1024;
    case 4:
        return 128 * 1024;
    case 5:
        return 64 * 1024;
    default:
        GB_LOG("unknown RAM size code %02X — assuming 0", code);
        return 0;
    }
}

static void parse_title(cartridge_t *c, const u8 *rom)
{
    size_t i;
    for (i = 0; i < GB_TITLE_LEN; i++) {
        const u8 ch = rom[0x134 + i];
        if (ch == 0 || ch < 32 || ch > 126) {
            c->title[i] = '\0';
            break;
        }
        c->title[i] = (char)ch;
    }
    c->title[GB_TITLE_LEN] = '\0';
}

bool cartridge_load(gb_t *gb, const u8 *data, size_t len)
{
    if (data == NULL || len < 0x150) {
        GB_LOG("ROM too small to contain a header");
        return false;
    }
    if (len > GB_MAX_ROM_BYTES) {
        GB_LOG("ROM exceeds 8 MiB cap");
        return false;
    }
    cartridge_unload(gb);

    const u8 type = data[0x147];
    const u8 rom_code = data[0x148];
    const u8 ram_code = data[0x149];
    mbc_init_from_type(gb, type);
    if (gb->cart.mbc.kind == GB_MBC_UNSUPPORTED) {
        GB_LOG("unsupported cartridge type %02X", type);
        return false;
    }

    size_t rom_alloc = rom_bytes_from_code(rom_code, len);
    if (rom_alloc < len) {
        rom_alloc = len;
    }
    if (rom_alloc > GB_MAX_ROM_BYTES) {
        return false;
    }
    gb->cart.rom = (u8 *)calloc(1, rom_alloc);
    if (gb->cart.rom == NULL) {
        return false;
    }
    (void)gb_memcpy_bounded(gb->cart.rom, rom_alloc, data, len);
    gb->cart.rom_size = rom_alloc;

    gb->cart.ram_size = ram_bytes_from_code(ram_code, gb->cart.mbc.kind);
    if (gb->cart.ram_size > GB_MAX_RAM_BYTES) {
        GB_LOG("RAM size rejected");
        cartridge_unload(gb);
        return false;
    }
    if (gb->cart.ram_size > 0) {
        gb->cart.ram = (u8 *)malloc(gb->cart.ram_size);
        if (gb->cart.ram == NULL) {
            cartridge_unload(gb);
            return false;
        }
        (void)memset(gb->cart.ram, 0xFF, gb->cart.ram_size);
    }

    gb->cart.cart_type = type;
    gb->cart.rom_size_code = rom_code;
    gb->cart.ram_size_code = ram_code;
    parse_title(&gb->cart, gb->cart.rom);

    u8 x = 0;
    u16 i;
    for (i = 0x134; i <= 0x14C; i++) {
        x = (u8)(x - gb->cart.rom[i] - 1u);
    }
    gb->cart.header_ok = (x == gb->cart.rom[0x14D]);
    if (!gb->cart.header_ok) {
        GB_LOG("header checksum mismatch (continuing)");
    }
    if (gb->cart.rom[0x104] != 0xCE || gb->cart.rom[0x105] != 0xED) {
        GB_LOG("Nintendo logo missing (continuing)");
    }
    gb->cart.mbc.rom_bank = 1;
    GB_LOG("loaded '%s' type=%02X rom=%zu ram=%zu", gb->cart.title, type, gb->cart.rom_size,
           gb->cart.ram_size);
    return true;
}

u8 cartridge_read(gb_t *gb, u16 addr)
{
    return mbc_read(gb, addr);
}

void cartridge_write(gb_t *gb, u16 addr, u8 value)
{
    mbc_write(gb, addr, value);
}
