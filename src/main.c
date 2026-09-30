/**
 * @file main.c
 * @brief SDL2 desktop frontend. The C core never includes SDL.
 *
 * Keybindings: arrows/WASD d-pad, Z/X A/B, Enter Start, RShift Select,
 * D debugger, Space pause, F5/F9 save/load state.
 */

#include "gb/gb.h"

#include <SDL.h>
#undef main
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void dbg_print(gb_t *gb)
{
    char line[128];
    u16 pc = gb->cpu.pc;
    int i;
    fprintf(stderr, "AF=%04X BC=%04X DE=%04X HL=%04X SP=%04X PC=%04X IME=%d LY=%u MODE=%u CYC=%llu\n",
            cpu_af(&gb->cpu), cpu_bc(&gb->cpu), cpu_de(&gb->cpu), cpu_hl(&gb->cpu), gb->cpu.sp,
            gb->cpu.pc, gb->cpu.ime ? 1 : 0, gb->ppu.ly, gb->ppu.mode,
            (unsigned long long)gb->cycles);
    for (i = 0; i < 10; i++) {
        (void)disassemble_at(gb, pc, line, sizeof line);
        fprintf(stderr, "  %s\n", line);
        const u8 op = mmu_read(gb, pc);
        extern const u8 gb_op_size[256];
        pc = (u16)(pc + (op == 0xCB ? 2 : gb_op_size[op]));
    }
}

int main(int argc, char **argv)
{
    int scale = 4;
    bool debug = false;
    const char *rom_path = NULL;
    int a;
    for (a = 1; a < argc; a++) {
        if (strcmp(argv[a], "--debug") == 0) {
            debug = true;
        } else if (strcmp(argv[a], "--scale") == 0 && a + 1 < argc) {
            scale = atoi(argv[++a]);
            if (scale < 1) {
                scale = 1;
            }
        } else if (argv[a][0] != '-') {
            rom_path = argv[a];
        }
    }
    if (rom_path == NULL) {
        fprintf(stderr, "usage: gb_desktop rom.gb [--debug] [--scale N]\n");
        return 1;
    }

    gb_t *gb = (gb_t *)calloc(1, sizeof *gb);
    if (gb == NULL) {
        return 1;
    }
    gb_init(gb);
    if (!gb_load_rom_file(gb, rom_path)) {
        gb_destroy(gb);
        free(gb);
        return 1;
    }
    gb->dbg.enabled = debug;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        GB_LOG("SDL_Init: %s", SDL_GetError());
        gb_destroy(gb);
        free(gb);
        return 1;
    }
    SDL_Window *win = SDL_CreateWindow("gb_desktop", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       GB_LCD_WIDTH * scale, GB_LCD_HEIGHT * scale, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                                         GB_LCD_WIDTH, GB_LCD_HEIGHT);
    bool quit = false;
    u32 last = SDL_GetTicks();
    while (!quit && gb->running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                const bool down = e.type == SDL_KEYDOWN;
                switch (e.key.keysym.sym) {
                case SDLK_RIGHT:
                case SDLK_d:
                    gb_set_button(gb, GB_BTN_RIGHT, down);
                    break;
                case SDLK_LEFT:
                case SDLK_a:
                    gb_set_button(gb, GB_BTN_LEFT, down);
                    break;
                case SDLK_UP:
                case SDLK_w:
                    gb_set_button(gb, GB_BTN_UP, down);
                    break;
                case SDLK_DOWN:
                case SDLK_s:
                    gb_set_button(gb, GB_BTN_DOWN, down);
                    break;
                case SDLK_z:
                case SDLK_j:
                case SDLK_SPACE:
                    gb_set_button(gb, GB_BTN_A, down);
                    break;
                case SDLK_x:
                case SDLK_k:
                    gb_set_button(gb, GB_BTN_B, down);
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    gb_set_button(gb, GB_BTN_START, down);
                    break;
                case SDLK_RSHIFT:
                case SDLK_LSHIFT:
                    gb_set_button(gb, GB_BTN_SELECT, down);
                    break;
                case SDLK_ESCAPE:
                    if (down) {
                        quit = true;
                    }
                    break;
                default:
                    break;
                }
                if (down && e.key.keysym.sym == SDLK_F12) {
                    gb->dbg.enabled = !gb->dbg.enabled;
                    dbg_print(gb);
                }
                if (down && e.key.keysym.sym == SDLK_p) {
                    gb->paused = !gb->paused;
                }
                if (down && e.key.keysym.sym == SDLK_F5) {
                    (void)gb_save_state(gb, "gb.state");
                }
                if (down && e.key.keysym.sym == SDLK_F9) {
                    (void)gb_load_state(gb, "gb.state");
                }
                if (down && e.key.keysym.sym == SDLK_n && gb->dbg.enabled) {
                    gb->paused = false;
                    gb->dbg.break_next = false;
                    (void)gb_step(gb);
                    gb->paused = true;
                    dbg_print(gb);
                }
            }
        }
        if (!gb->paused) {
            gb_run_frame(gb);
        }
        SDL_UpdateTexture(tex, NULL, gb_framebuffer(gb), GB_LCD_WIDTH * 4);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
        const u32 now = SDL_GetTicks();
        if (now - last < 16) {
            SDL_Delay(16 - (now - last));
        }
        last = SDL_GetTicks();
    }
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    gb_destroy(gb);
    free(gb);
    return 0;
}
