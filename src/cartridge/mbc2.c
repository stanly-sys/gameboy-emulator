/**
 * @file mbc2.c
 * @brief MBC2: 16 ROM banks, 512 nibble RAM, enable via bit 8 of address.
 */

#include "gb/gb.h"

u8 mbc2_read(gb_t *gb, u16 addr)
{
    if (addr < 0x4000) {
        return (gb->cart.rom != NULL && addr < gb->cart.rom_size) ? gb->cart.rom[addr] : 0xFF;
    }
    if (addr < 0x8000) {
        u16 bank = gb->cart.mbc.rom_bank & 0x0Fu;
        if (bank == 0) {
            bank = 1;
        }
        const size_t off = (size_t)bank * 0x4000u + (size_t)(addr - 0x4000);
        if (gb->cart.rom == NULL || off >= gb->cart.rom_size) {
            return 0xFF;
        }
        return gb->cart.rom[off];
    }
    if (addr >= 0xA000 && addr < 0xA200 && gb->cart.mbc.ram_enable && gb->cart.ram != NULL) {
        const size_t o = (size_t)(addr - 0xA000) % gb->cart.ram_size;
        return (u8)(gb->cart.ram[o] | 0xF0u);
    }
    return 0xFF;
}

void mbc2_write(gb_t *gb, u16 addr, u8 value)
{
    if (addr < 0x4000) {
        if ((addr & 0x0100u) == 0) {
            gb->cart.mbc.ram_enable = (value & 0x0Fu) == 0x0A;
        } else {
            u8 b = (u8)(value & 0x0Fu);
            if (b == 0) {
                b = 1;
            }
            gb->cart.mbc.rom_bank = b;
        }
        return;
    }
    if (addr >= 0xA000 && addr < 0xA200 && gb->cart.mbc.ram_enable && gb->cart.ram != NULL) {
        const size_t o = (size_t)(addr - 0xA000) % gb->cart.ram_size;
        gb->cart.ram[o] = (u8)(value & 0x0Fu);
    }
}
