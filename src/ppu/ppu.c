/**
 * @file ppu.c
 * @brief LCD mode machine and scanline kickoff.
 *
 * Hardware reference: 456 dots/line, 154 lines, modes 2→3→0 then VBlank.
 * Cycle model: 4 dots per T-cycle.
 * Thread-safety: not thread-safe.
 * Error handling: LCD off freezes LY at 0.
 */

#include "gb/gb.h"

void ppu_set_classic_green_palette(gb_t *gb)
{
    gb->ppu.palette_rgba[0] = 0xFF9BBC0Fu; /* lightest green, RGBA */
    gb->ppu.palette_rgba[1] = 0xFF8BAC0Fu;
    gb->ppu.palette_rgba[2] = 0xFF306230u;
    gb->ppu.palette_rgba[3] = 0xFF0F380Fu; /* darkest green, RGBA */
}

void ppu_init(gb_t *gb)
{
    ppu_reset(gb);
    ppu_set_classic_green_palette(gb);
}

void ppu_reset(gb_t *gb)
{
    gb->ppu.mode = GB_PPU_MODE_OAM;
    gb->ppu.dots = 0;
    gb->ppu.ly = 0;
    gb->ppu.lyc = 0;
    gb->ppu.lcdc = 0x91;
    gb->ppu.stat = 0x85;
    gb->ppu.scy = 0;
    gb->ppu.scx = 0;
    gb->ppu.wy = 0;
    gb->ppu.wx = 0;
    gb->ppu.bgp = 0xFC;
    gb->ppu.obp0 = 0xFF;
    gb->ppu.obp1 = 0xFF;
    gb->ppu.lyc_stat_prev = false;
    gb->ppu.window_internal_line = 0;
}

u8 ppu_stat_mode_bits(const gb_t *gb)
{
    u8 st = (u8)((gb->ppu.stat & 0x78u) | (gb->ppu.mode & 3u));
    if (gb->ppu.ly == gb->ppu.lyc) {
        st = (u8)(st | 0x04u);
    }
    return (u8)(st | 0x80u);
}

void ppu_lyc_check(gb_t *gb)
{
    const bool eq = (gb->ppu.ly == gb->ppu.lyc);
    if (eq && (gb->ppu.stat & 0x40u) != 0 && !gb->ppu.lyc_stat_prev) {
        interrupts_request(gb, GB_INT_STAT);
    }
    gb->ppu.lyc_stat_prev = eq;
}

static void ppu_enter_mode(gb_t *gb, u8 mode)
{
    gb->ppu.mode = mode;
    if (mode == GB_PPU_MODE_HBLANK && (gb->ppu.stat & 0x08u) != 0) {
        interrupts_request(gb, GB_INT_STAT);
    }
    if (mode == GB_PPU_MODE_VBLANK && (gb->ppu.stat & 0x10u) != 0) {
        interrupts_request(gb, GB_INT_STAT);
    }
    if (mode == GB_PPU_MODE_OAM && (gb->ppu.stat & 0x20u) != 0) {
        interrupts_request(gb, GB_INT_STAT);
    }
}

void ppu_step(gb_t *gb, u32 t_cycles)
{
    if ((gb->ppu.lcdc & 0x80u) == 0) {
        return;
    }
    u32 dots = t_cycles;
    while (dots > 0) {
        gb->ppu.dots++;
        dots--;
        if (gb->ppu.ly < GB_LCD_HEIGHT) {
            if (gb->ppu.dots == 1) {
                ppu_enter_mode(gb, GB_PPU_MODE_OAM);
            } else if (gb->ppu.dots == 81) {
                ppu_enter_mode(gb, GB_PPU_MODE_VRAM);
            } else if (gb->ppu.dots == 253) {
                ppu_render_scanline(gb, gb->ppu.ly);
                ppu_enter_mode(gb, GB_PPU_MODE_HBLANK);
            }
        }
        if (gb->ppu.dots >= GB_DOTS_PER_LINE) {
            gb->ppu.dots = 0;
            gb->ppu.ly++;
            if (gb->ppu.ly == GB_LCD_HEIGHT) {
                ppu_enter_mode(gb, GB_PPU_MODE_VBLANK);
                interrupts_request(gb, GB_INT_VBLANK);
            }
            if (gb->ppu.ly >= GB_LCD_TOTAL_LINES) {
                gb->ppu.ly = 0;
                gb->ppu.window_internal_line = 0;
            }
            ppu_lyc_check(gb);
        }
    }
}

u32 ppu_shade(const gb_t *gb, u8 pal, u8 color_id)
{
    const u8 idx = (u8)((pal >> (u8)(color_id * 2u)) & 3u);
    return gb->ppu.palette_rgba[idx];
}

void ppu_put_pixel(gb_t *gb, u8 x, u8 y, u32 rgba)
{
    if (x >= GB_LCD_WIDTH || y >= GB_LCD_HEIGHT) {
        return;
    }
    const size_t o = ((size_t)y * GB_LCD_WIDTH + (size_t)x) * 4u;
    gb->framebuffer[o + 0] = (u8)((rgba >> 16) & 0xFFu); /* R */
    gb->framebuffer[o + 1] = (u8)((rgba >> 8) & 0xFFu);
    gb->framebuffer[o + 2] = (u8)(rgba & 0xFFu);
    gb->framebuffer[o + 3] = 0xFF;
}

u8 ppu_tile_color(gb_t *gb, u16 tile_addr, u8 px, u8 py)
{
    const u8 lo = gb->mmu.vram[(tile_addr + (u16)(py * 2u)) - 0x8000];
    const u8 hi = gb->mmu.vram[(tile_addr + (u16)(py * 2u) + 1u) - 0x8000];
    const u8 bit = (u8)(7u - px);
    const u8 c0 = (u8)((lo >> bit) & 1u);
    const u8 c1 = (u8)((hi >> bit) & 1u);
    return (u8)(c0 | (u8)(c1 << 1));
}

void ppu_render_scanline(gb_t *gb, u8 y)
{
    u8 bg_color[GB_LCD_WIDTH];
    u8 x;
    for (x = 0; x < GB_LCD_WIDTH; x++) {
        bg_color[x] = 0;
        ppu_put_pixel(gb, x, y, ppu_shade(gb, gb->ppu.bgp, 0));
    }
    ppu_draw_background_line(gb, y, bg_color);
    ppu_draw_window_line(gb, y, bg_color);
    ppu_draw_sprites_line(gb, y, bg_color);
}
