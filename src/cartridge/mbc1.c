/**
 * @file mbc1.c
 * @brief MBC1 ROM/RAM banking, mode 0 (16/8) and mode 1 (4/32).
 */

#include "gb/gb.h"

static size_t bank_mask(size_t rom_size)
{
    size_t banks = rom_size / 0x4000u;
    if (banks == 0) {
        banks = 1;
    }
    size_t m = 1;
    while (m < banks) {
        m <<= 1;
    }
    return m - 1u;
}

u8 mbc1_read(gb_t *gb, u16 addr)
{
    if (addr < 0x4000) {
        size_t bank0 = 0;
        if (gb->cart.mbc.mbc1_mode == 1) {
            bank0 = (size_t)(gb->cart.mbc.mbc1_bank2 << 5) & bank_mask(gb->cart.rom_size);
        }
        const size_t off = bank0 * 0x4000u + addr;
        if (off >= gb->cart.rom_size) {
            return 0xFF;
        }
        return gb->cart.rom[off];
    }
    if (addr < 0x8000) {
        u16 bank = gb->cart.mbc.rom_bank;
        if (bank == 0) {
            bank = 1;
        }
        bank = (u16)(bank | (u16)(gb->cart.mbc.mbc1_bank2 << 5));
        bank = (u16)(bank & (u16)bank_mask(gb->cart.rom_size));
        if (bank == 0) {
            bank = 1;
        }
        const size_t off = (size_t)bank * 0x4000u + (size_t)(addr - 0x4000);
        if (off >= gb->cart.rom_size) {
            return 0xFF;
        }
        return gb->cart.rom[off];
    }
    if (addr >= 0xA000 && addr < 0xC000 && gb->cart.mbc.ram_enable && gb->cart.ram != NULL) {
        size_t rb = 0;
        if (gb->cart.mbc.mbc1_mode == 1) {
            rb = gb->cart.mbc.ram_bank;
        }
        const size_t off = rb * 0x2000u + (size_t)(addr - 0xA000);
        if (off >= gb->cart.ram_size) {
            return 0xFF;
        }
        return gb->cart.ram[off];
    }
    return 0xFF;
}

void mbc1_write(gb_t *gb, u16 addr, u8 value)
{
    if (addr < 0x2000) {
        gb->cart.mbc.ram_enable = (value & 0x0Fu) == 0x0A;
        return;
    }
    if (addr < 0x4000) {
        u8 lo = (u8)(value & 0x1Fu);
        if (lo == 0) {
            lo = 1;
        }
        gb->cart.mbc.rom_bank = (u16)((gb->cart.mbc.rom_bank & 0x60u) | lo);
        return;
    }
    if (addr < 0x6000) {
        gb->cart.mbc.mbc1_bank2 = (u8)(value & 0x03u);
        gb->cart.mbc.ram_bank = gb->cart.mbc.mbc1_bank2;
        return;
    }
    if (addr < 0x8000) {
        gb->cart.mbc.mbc1_mode = (u8)(value & 0x01u);
        return;
    }
    if (addr >= 0xA000 && addr < 0xC000 && gb->cart.mbc.ram_enable && gb->cart.ram != NULL) {
        size_t rb = 0;
        if (gb->cart.mbc.mbc1_mode == 1) {
            rb = gb->cart.mbc.ram_bank;
        }
        const size_t off = rb * 0x2000u + (size_t)(addr - 0xA000);
        if (off < gb->cart.ram_size) {
            gb->cart.ram[off] = value;
        }
    }
}
