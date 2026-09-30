/**
 * @file ppu_bg.c
 * @brief Background tilemap fetch for one scanline.
 *
 * Hardware reference: LCDC.3 map, LCDC.4 addressing, SCX/SCY wrap.
 * Cycle model: none (whole line at Mode 3 end).
 * Thread-safety: not thread-safe.
 * Error handling: LCDC.0 off on DMG blanks the BG to color 0.
 */

#include "gb/gb.h"

void ppu_draw_background_line(gb_t *gb, u8 y, u8 *bg_color)
{
    if ((gb->ppu.lcdc & 0x01u) == 0) {
        return;
    }
    const u16 map_base = ((gb->ppu.lcdc & 0x08u) != 0) ? 0x9C00u : 0x9800u;
    const bool signed_tiles = (gb->ppu.lcdc & 0x10u) == 0;
    u8 x;
    for (x = 0; x < GB_LCD_WIDTH; x++) {
        const u8 lx = (u8)(x + gb->ppu.scx);
        const u8 ly = (u8)(y + gb->ppu.scy);
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
