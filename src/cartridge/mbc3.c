/**
 * @file mbc3.c
 * @brief MBC3 ROM/RAM plus 5 RTC registers (latched snapshot only).
 */

#include "gb/gb.h"

u8 mbc3_read(gb_t *gb, u16 addr)
{
    if (addr < 0x4000) {
        return (gb->cart.rom != NULL && addr < gb->cart.rom_size) ? gb->cart.rom[addr] : 0xFF;
    }
    if (addr < 0x8000) {
        u16 bank = gb->cart.mbc.rom_bank & 0x7Fu;
        if (bank == 0) {
            bank = 1;
        }
        const size_t off = (size_t)bank * 0x4000u + (size_t)(addr - 0x4000);
        if (gb->cart.rom == NULL || off >= gb->cart.rom_size) {
            return 0xFF;
        }
        return gb->cart.rom[off];
    }
    if (addr >= 0xA000 && addr < 0xC000 && gb->cart.mbc.ram_enable) {
        if (gb->cart.mbc.ram_bank <= 3 && gb->cart.ram != NULL) {
            const size_t off =
                (size_t)gb->cart.mbc.ram_bank * 0x2000u + (size_t)(addr - 0xA000);
            if (off < gb->cart.ram_size) {
                return gb->cart.ram[off];
            }
        }
        if (gb->cart.mbc.ram_bank >= 0x08 && gb->cart.mbc.ram_bank <= 0x0C) {
            return gb->cart.mbc.rtc[gb->cart.mbc.ram_bank - 0x08];
        }
    }
    return 0xFF;
}

void mbc3_write(gb_t *gb, u16 addr, u8 value)
{
    if (addr < 0x2000) {
        gb->cart.mbc.ram_enable = (value & 0x0Fu) == 0x0A;
        return;
    }
    if (addr < 0x4000) {
        u8 b = (u8)(value & 0x7Fu);
        if (b == 0) {
            b = 1;
        }
        gb->cart.mbc.rom_bank = b;
        return;
    }
    if (addr < 0x6000) {
        gb->cart.mbc.ram_bank = value;
        return;
    }
    if (addr < 0x8000) {
        if (!gb->cart.mbc.rtc_latch && (value & 1u) != 0) {
            /* Host time is not wired; RTC stays at last written values. */
        }
        gb->cart.mbc.rtc_latch = (value & 1u) != 0;
        return;
    }
    if (addr >= 0xA000 && addr < 0xC000 && gb->cart.mbc.ram_enable) {
        if (gb->cart.mbc.ram_bank <= 3 && gb->cart.ram != NULL) {
            const size_t off =
                (size_t)gb->cart.mbc.ram_bank * 0x2000u + (size_t)(addr - 0xA000);
            if (off < gb->cart.ram_size) {
                gb->cart.ram[off] = value;
            }
        } else if (gb->cart.mbc.ram_bank >= 0x08 && gb->cart.mbc.ram_bank <= 0x0C) {
            gb->cart.mbc.rtc[gb->cart.mbc.ram_bank - 0x08] = value;
        }
    }
}
