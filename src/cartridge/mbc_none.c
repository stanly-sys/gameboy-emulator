/**
 * @file mbc_none.c
 * @brief 32 KiB ROM, optional RAM at A000 with no banking.
 */

#include "gb/gb.h"

static u8 rom_at(const gb_t *gb, size_t off)
{
    if (gb->cart.rom == NULL || off >= gb->cart.rom_size) {
        return 0xFF;
    }
    return gb->cart.rom[off];
}

u8 mbc_none_read(gb_t *gb, u16 addr)
{
    if (addr < 0x8000) {
        return rom_at(gb, addr);
    }
    if (addr >= 0xA000 && addr < 0xC000 && gb->cart.ram != NULL && gb->cart.ram_size > 0) {
        const size_t o = (size_t)(addr - 0xA000) % gb->cart.ram_size;
        return gb->cart.ram[o];
    }
    return 0xFF;
}

void mbc_none_write(gb_t *gb, u16 addr, u8 value)
{
    if (addr >= 0xA000 && addr < 0xC000 && gb->cart.ram != NULL && gb->cart.ram_size > 0) {
        const size_t o = (size_t)(addr - 0xA000) % gb->cart.ram_size;
        gb->cart.ram[o] = value;
        return;
    }
    GB_LOG("ROM write %02X -> %04X ignored (no MBC)", value, addr);
}

size_t mbc_rom_offset(const gb_t *gb, u16 addr)
{
    (void)gb;
    return addr;
}

size_t mbc_ram_offset(const gb_t *gb, u16 addr)
{
    (void)gb;
    return (size_t)(addr - 0xA000);
}
