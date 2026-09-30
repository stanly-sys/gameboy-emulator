/**
 * @file test_ppu.c
 * @brief Palette and LCD-off freeze.
 */

#include "gb/gb.h"
#include "test.h"

void test_ppu(void)
{
    gb_t gb;
    gb_init(&gb);
    GB_CHECK(gb.ppu.palette_rgba[0] != 0);
    gb.ppu.lcdc = 0;
    const u8 ly = gb.ppu.ly;
    ppu_step(&gb, 100);
    GB_CHECK(gb.ppu.ly == ly);
    gb_destroy(&gb);
}
