/**
 * @file ppu_window.c
 * @brief Window layer (WEL, WX-7, WY) using its own internal line counter.
 *
 * Hardware reference: LCDC.5 enable, LCDC.6 map.
 * Cycle model: none.
 * Thread-safety: not thread-safe.
 * Error handling: WX<7 still draws with wrap into negative off-screen skip.
 */

#include "gb/gb.h"

void ppu_draw_window_line(gb_t *gb, u8 y, u8 *bg_color)
{
    if ((gb->ppu.lcdc & 0x20u) == 0) {
        return;
    }
    if (y < gb->ppu.wy) {
        return;
    }
    const int wx = (int)gb->ppu.wx - 7;
    if (wx >= GB_LCD_WIDTH) {
        return;
    }
    const u16 map_base = ((gb->ppu.lcdc & 0x40u) != 0) ? 0x9C00u : 0x9800u;
    const bool signed_tiles = (gb->ppu.lcdc & 0x10u) == 0;
    const u8 wyi = gb->ppu.window_internal_line;
    gb->ppu.window_internal_line = (u8)(gb->ppu.window_internal_line + 1u);
    u8 x;
    for (x = 0; x < GB_LCD_WIDTH; x++) {
        if ((int)x < wx) {
            continue;
        }
        const u8 lx = (u8)((int)x - wx);
        const u8 ly = wyi;
        const u16 tx = (u16)(lx / 8u);
        const u16 ty = (u16)(ly / 8u);
        const u16 map = (u16)(map_base + ty * 32u + tx);
        const u8 tile = gb->mmu.vram[map - 0x8000u];
        u16 tile_addr;
        if (!signed_tiles) {
            tile_addr = (u16)(0x8000u + (u16)tile * 16u);
        } else {
            tile_addr = (u16)(0x9000 + (int)(i8)tile * 16);
        }
        const u8 col = ppu_tile_color(gb, tile_addr, (u8)(lx % 8u), (u8)(ly % 8u));
        bg_color[x] = col;
        ppu_put_pixel(gb, x, y, ppu_shade(gb, gb->ppu.bgp, col));
    }
}
