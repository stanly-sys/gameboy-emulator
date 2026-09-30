/**
 * @file mbc.c
 * @brief Map cartridge type byte to an MBC implementation.
 */

#include "gb/gb.h"

void mbc_init_from_type(gb_t *gb, u8 cart_type)
{
    (void)memset(&gb->cart.mbc, 0, sizeof gb->cart.mbc);
    gb->cart.mbc.rom_bank = 1;
    switch (cart_type) {
    case 0x00:
    case 0x08:
    case 0x09:
        gb->cart.mbc.kind = GB_MBC_NONE;
        break;
    case 0x01:
    case 0x02:
    case 0x03:
        gb->cart.mbc.kind = GB_MBC_MBC1;
        break;
    case 0x05:
    case 0x06:
        gb->cart.mbc.kind = GB_MBC_MBC2;
        break;
    case 0x0F:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
        gb->cart.mbc.kind = GB_MBC_MBC3;
        break;
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
        gb->cart.mbc.kind = GB_MBC_MBC5;
        break;
    default:
        gb->cart.mbc.kind = GB_MBC_UNSUPPORTED;
        break;
    }
}

u8 mbc_read(gb_t *gb, u16 addr)
{
    switch (gb->cart.mbc.kind) {
    case GB_MBC_NONE:
        return mbc_none_read(gb, addr);
    case GB_MBC_MBC1:
        return mbc1_read(gb, addr);
    case GB_MBC_MBC2:
        return mbc2_read(gb, addr);
    case GB_MBC_MBC3:
        return mbc3_read(gb, addr);
    case GB_MBC_MBC5:
        return mbc5_read(gb, addr);
    default:
        return 0xFF;
    }
}

void mbc_write(gb_t *gb, u16 addr, u8 value)
{
    switch (gb->cart.mbc.kind) {
    case GB_MBC_NONE:
        mbc_none_write(gb, addr, value);
        break;
    case GB_MBC_MBC1:
        mbc1_write(gb, addr, value);
        break;
    case GB_MBC_MBC2:
        mbc2_write(gb, addr, value);
        break;
    case GB_MBC_MBC3:
        mbc3_write(gb, addr, value);
        break;
    case GB_MBC_MBC5:
        mbc5_write(gb, addr, value);
        break;
    default:
        GB_LOG("MBC write on unsupported mapper");
        break;
    }
}
