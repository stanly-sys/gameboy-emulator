/**
 * @file ppu.h
 * @brief DMG Picture Processing Unit: modes, scanlines, framebuffer.
 *
 * Hardware reference: LCDC/STAT/LY, 456 dots/line, 154 lines/frame.
 * Cycle model: 4 PPU dots per CPU T-cycle.
 * Thread-safety: not thread-safe.
 * Error handling: LCD-off forces LY=0 and mode 0.
 */

#pragma once

#include "gb/types.h"

struct gb;

enum {
    GB_LCD_WIDTH = 160,
    GB_LCD_HEIGHT = 144,
    GB_LCD_VBLANK_LINES = 10,
    GB_LCD_TOTAL_LINES = 154,
    GB_DOTS_PER_LINE = 456,
    GB_DOTS_PER_FRAME = 70224,
    GB_PPU_MODE_HBLANK = 0,
    GB_PPU_MODE_VBLANK = 1,
    GB_PPU_MODE_OAM = 2,
    GB_PPU_MODE_VRAM = 3
};

typedef struct {
    u8 mode;
    u16 dots; /**< Dots elapsed in the current scanline. */
    u8 ly;
    u8 lyc;
    u8 lcdc;
    u8 stat;
    u8 scy;
    u8 scx;
    u8 wy;
    u8 wx;
    u8 bgp;
    u8 obp0;
    u8 obp1;
    bool lyc_stat_prev; /**< Edge detect for STAT LYC interrupt. */
    u32 palette_rgba[4]; /**< Configurable DMG shades, RGBA8. */
    bool window_line_active;
    u8 window_internal_line;
} ppu_t;

void ppu_init(struct gb *gb);
void ppu_reset(struct gb *gb);
void ppu_step(struct gb *gb, u32 t_cycles);
void ppu_lyc_check(struct gb *gb);
void ppu_render_scanline(struct gb *gb, u8 y);
void ppu_set_classic_green_palette(struct gb *gb);
u8 ppu_stat_mode_bits(const struct gb *gb);
void ppu_put_pixel(struct gb *gb, u8 x, u8 y, u32 rgba);
u8 ppu_tile_color(struct gb *gb, u16 tile_addr, u8 px, u8 py);
u32 ppu_shade(const struct gb *gb, u8 pal, u8 color_id);
void ppu_draw_background_line(struct gb *gb, u8 y, u8 *bg_color);
void ppu_draw_window_line(struct gb *gb, u8 y, u8 *bg_color);
void ppu_draw_sprites_line(struct gb *gb, u8 y, const u8 *bg_color);
