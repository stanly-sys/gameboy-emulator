/**
 * @file mbc5.c
 * @brief MBC5: 9-bit ROM bank, 4-bit RAM bank, optional rumble bit.
 */

#include "gb/gb.h"

u8 mbc5_read(gb_t *gb, u16 addr)
{
    if (addr < 0x4000) {
        return (gb->cart.rom != NULL && addr < gb->cart.rom_size) ? gb->cart.rom[addr] : 0xFF;
    }
    if (addr < 0x8000) {
        const u16 bank = gb->cart.mbc.rom_bank;
        const size_t off = (size_t)bank * 0x4000u + (size_t)(addr - 0x4000);
        if (gb->cart.rom == NULL || off >= gb->cart.rom_size) {
            return 0xFF;
        }
        return gb->cart.rom[off];
    }
    if (addr >= 0xA000 && addr < 0xC000 && gb->cart.mbc.ram_enable && gb->cart.ram != NULL) {
        const size_t off =
            (size_t)(gb->cart.mbc.ram_bank & 0x0Fu) * 0x2000u + (size_t)(addr - 0xA000);
        if (off < gb->cart.ram_size) {
            return gb->cart.ram[off];
        }
    }
    return 0xFF;
}

void mbc5_write(gb_t *gb, u16 addr, u8 value)
{
    if (addr < 0x2000) {
        gb->cart.mbc.ram_enable = (value & 0x0Fu) == 0x0A;
        return;
    }
    if (addr < 0x3000) {
        gb->cart.mbc.rom_bank = (u16)((gb->cart.mbc.rom_bank & 0x100u) | value);
        return;
    }
    if (addr < 0x4000) {
        gb->cart.mbc.rom_bank = (u16)((gb->cart.mbc.rom_bank & 0xFFu) | ((u16)(value & 1u) << 8));
        return;
    }
    if (addr < 0x6000) {
        gb->cart.mbc.ram_bank = (u8)(value & 0x0Fu);
        gb->cart.mbc.rumble = (u8)((value >> 3) & 1u);
        return;
    }
    if (addr >= 0xA000 && addr < 0xC000 && gb->cart.mbc.ram_enable && gb->cart.ram != NULL) {
        const size_t off =
            (size_t)(gb->cart.mbc.ram_bank & 0x0Fu) * 0x2000u + (size_t)(addr - 0xA000);
        if (off < gb->cart.ram_size) {
            gb->cart.ram[off] = value;
        }
    }
}
