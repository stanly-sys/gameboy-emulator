/**
 * @file ppu_sprite.c
 * @brief OAM sprites: 8x8 / 8x16, 10 per line, X-priority on DMG.
 *
 * Hardware reference: OAM 4-byte entries, LCDC.1/LCDC.2.
 * Cycle model: none.
 * Thread-safety: not thread-safe.
 * Error handling: color 0 is transparent; BG priority bit hides over BG>0.
 */

#include "gb/gb.h"

void ppu_draw_sprites_line(gb_t *gb, u8 y, const u8 *bg_color)
{
    if ((gb->ppu.lcdc & 0x02u) == 0) {
        return;
    }
    const int height = ((gb->ppu.lcdc & 0x04u) != 0) ? 16 : 8;
    int drawn = 0;
    /* Collect up to 10 sprites, then sort by X (lowest X first = drawn later = on top). */
    int idx[10];
    int n = 0;
    int i;
    for (i = 0; i < 40 && n < 10; i++) {
        const u8 *o = &gb->mmu.oam[(size_t)i * 4u];
        const int sy = (int)o[0] - 16;
        if ((int)y >= sy && (int)y < sy + height) {
            idx[n++] = i;
        }
    }
    /* Insertion sort by X then OAM index (stable). */
    for (i = 1; i < n; i++) {
        const int key = idx[i];
        int j = i;
        while (j > 0) {
            const u8 xa = gb->mmu.oam[(size_t)idx[j - 1] * 4u + 1u];
            const u8 xb = gb->mmu.oam[(size_t)key * 4u + 1u];
            if (xa <= xb) {
                break;
            }
            idx[j] = idx[j - 1];
            j--;
        }
        idx[j] = key;
    }
    /* Draw from rightmost (lowest priority) so leftmost ends on top. */
    for (i = n - 1; i >= 0; i--) {
        const u8 *o = &gb->mmu.oam[(size_t)idx[i] * 4u];
        const int sy = (int)o[0] - 16;
        const int sx = (int)o[1] - 8;
        u8 tile = o[2];
        const u8 attr = o[3];
        if (height == 16) {
            tile = (u8)(tile & 0xFEu);
        }
        int row = (int)y - sy;
        if ((attr & 0x40u) != 0) {
            row = height - 1 - row;
        }
        if (height == 16 && row >= 8) {
            tile = (u8)(tile | 1u);
            row -= 8;
        }
        const u16 tile_addr = (u16)(0x8000u + (u16)tile * 16u);
        const u8 pal = ((attr & 0x10u) != 0) ? gb->ppu.obp1 : gb->ppu.obp0;
        int px;
        for (px = 0; px < 8; px++) {
            int colx = px;
            if ((attr & 0x20u) != 0) {
                colx = 7 - px;
            }
            const int dx = sx + px;
            if (dx < 0 || dx >= GB_LCD_WIDTH) {
                continue;
            }
            const u8 col = ppu_tile_color(gb, tile_addr, (u8)colx, (u8)row);
            if (col == 0) {
                continue;
            }
            if ((attr & 0x80u) != 0 && bg_color[dx] != 0) {
                continue;
            }
            ppu_put_pixel(gb, (u8)dx, y, ppu_shade(gb, pal, col));
        }
        drawn++;
        (void)drawn;
    }
}
