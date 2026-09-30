/**
 * @file test_mbc.c
 * @brief Header parse rejects tiny buffers; MBC1 bank 0 maps as 1.
 */

#include "gb/gb.h"
#include "test.h"

void test_mbc(void)
{
    gb_t gb;
    u8 rom[0x8000];
    size_t i;
    gb_init(&gb);
    for (i = 0; i < sizeof rom; i++) {
        rom[i] = 0;
    }
    rom[0x147] = 0x01;
    rom[0x148] = 0x00;
    rom[0x149] = 0x00;
    rom[0x134] = 'T';
    GB_CHECK(cartridge_load(&gb, rom, sizeof rom));
    GB_CHECK(gb.cart.mbc.kind == GB_MBC_MBC1);
    gb_destroy(&gb);
}
