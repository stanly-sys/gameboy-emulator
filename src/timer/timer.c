/**
 * @file timer.c
 * @brief DIV and TIMA increment from an internal 16-bit divider.
 *
 * Hardware reference: TAC clock select; overflow requests timer IRQ.
 * Cycle model: one internal tick per T-cycle.
 * Thread-safety: not thread-safe.
 * Error handling: writing DIV resets the whole divider (DIV-APU edge).
 */

#include "gb/gb.h"

static u16 timer_tac_bit(u8 tac)
{
    switch (tac & 0x03u) {
    case 0:
        return 9; /* 4096 Hz: bit 9 of divider (falling). */
    case 1:
        return 3; /* 262144 Hz */
    case 2:
        return 5; /* 65536 Hz */
    case 3:
        return 7; /* 16384 Hz */
    default:
        GB_LOG("timer: impossible TAC select");
        return 9;
    }
}

static bool timer_enabled(const timer_t *t)
{
    return (t->tac & 0x04u) != 0;
}

void timer_init(gb_t *gb)
{
    timer_reset(gb);
}

void timer_reset(gb_t *gb)
{
    gb->timer.div_counter = 0;
    gb->timer.tima = 0;
    gb->timer.tma = 0;
    gb->timer.tac = 0;
    gb->timer.tima_reload_delay = 0;
}

static void timer_tick_one(gb_t *gb)
{
    timer_t *t = &gb->timer;
    if (t->tima_reload_delay > 0) {
        t->tima_reload_delay--;
        if (t->tima_reload_delay == 0) {
            t->tima = t->tma;
            interrupts_request(gb, GB_INT_TIMER);
        }
    }

    const u16 old = t->div_counter;
    t->div_counter = (u16)(t->div_counter + 1u);
    if (!timer_enabled(t)) {
        return;
    }
    const u16 bit = timer_tac_bit(t->tac);
    const u16 mask = (u16)(1u << bit);
    /* Falling edge of the selected divider bit clocks TIMA. */
    if ((old & mask) != 0 && (t->div_counter & mask) == 0) {
        if (t->tima == 0xFF) {
            t->tima = 0;
            t->tima_reload_delay = 4; /* WHY: TIMA is 0 for 4 T-cycles then TMA. */
        } else if (t->tima_reload_delay == 0) {
            t->tima = (u8)(t->tima + 1u);
        }
    }
}

void timer_step(gb_t *gb, u32 t_cycles)
{
    u32 i;
    for (i = 0; i < t_cycles; i++) {
        timer_tick_one(gb);
    }
}

u8 timer_read(gb_t *gb, u16 addr)
{
    switch (addr) {
    case 0xFF04:
        return (u8)(gb->timer.div_counter >> 8);
    case 0xFF05:
        return gb->timer.tima;
    case 0xFF06:
        return gb->timer.tma;
    case 0xFF07:
        return (u8)(gb->timer.tac | 0xF8u);
    default:
        GB_LOG("timer_read: bad addr %04X", addr);
        return 0xFF;
    }
}

void timer_write(gb_t *gb, u16 addr, u8 value)
{
    switch (addr) {
    case 0xFF04:
        /* DIV-APU / TIMA glitch: resetting DIV can fall the TAC bit. */
        {
            const u16 old = gb->timer.div_counter;
            gb->timer.div_counter = 0;
            if (timer_enabled(&gb->timer)) {
                const u16 bit = timer_tac_bit(gb->timer.tac);
                const u16 mask = (u16)(1u << bit);
                if ((old & mask) != 0) {
                    if (gb->timer.tima == 0xFF) {
                        gb->timer.tima = 0;
                        gb->timer.tima_reload_delay = 4;
                    } else if (gb->timer.tima_reload_delay == 0) {
                        gb->timer.tima = (u8)(gb->timer.tima + 1u);
                    }
                }
            }
        }
        break;
    case 0xFF05:
        if (gb->timer.tima_reload_delay != 1) {
            gb->timer.tima = value;
        }
        gb->timer.tima_reload_delay = 0;
        break;
    case 0xFF06:
        gb->timer.tma = value;
        break;
    case 0xFF07:
        gb->timer.tac = (u8)(value & 0x07u);
        break;
    default:
        GB_LOG("timer_write: bad addr %04X", addr);
        break;
    }
}
