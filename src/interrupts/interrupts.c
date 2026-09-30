/**
 * @file interrupts.c
 * @brief Interrupt request flags and ISR dispatch (priority VBlank→Joypad).
 *
 * Hardware reference: IF/IE; vectors 0x40, 0x48, 0x50, 0x58, 0x60.
 * Cycle model: 20 T-cycles to enter an ISR.
 * Thread-safety: not thread-safe.
 * Error handling: no ISR if IME=0; HALT exits when IF&IE != 0.
 */

#include "gb/gb.h"

void interrupts_init(gb_t *gb)
{
    interrupts_reset(gb);
}

void interrupts_reset(gb_t *gb)
{
    gb->irq.iff = 0xE0; /* unused bits read as 1 on DMG */
}

void interrupts_request(gb_t *gb, u8 mask)
{
    gb->irq.iff = (u8)(gb->irq.iff | mask | 0xE0);
}

u8 interrupts_if_read(const gb_t *gb)
{
    return (u8)(gb->irq.iff | 0xE0);
}

void interrupts_if_write(gb_t *gb, u8 value)
{
    gb->irq.iff = (u8)((value & 0x1Fu) | 0xE0);
}

bool interrupts_pending(const gb_t *gb)
{
    return ((gb->irq.iff & gb->mmu.ie) & 0x1Fu) != 0;
}

u32 interrupts_service(gb_t *gb)
{
    if (!gb->cpu.ime) {
        return 0;
    }
    const u8 pending = (u8)((gb->irq.iff & gb->mmu.ie) & 0x1Fu);
    if (pending == 0) {
        return 0;
    }

    static const u16 vectors[5] = {0x0040, 0x0048, 0x0050, 0x0058, 0x0060};
    u8 bit = 0;
    for (bit = 0; bit < 5; bit++) {
        if ((pending & (u8)(1u << bit)) != 0) {
            break;
        }
    }
    if (bit >= 5) {
        return 0;
    }

    gb->cpu.ime = false;
    gb->cpu.halted = false;
    gb->cpu.halt_bug = false;
    gb->irq.iff = (u8)((gb->irq.iff & (u8)~(1u << bit)) | 0xE0);

    /* Push PC then jump — WHY: ISR prologue is five M-cycles on LR35902. */
    gb->cpu.sp = (u16)(gb->cpu.sp - 1u);
    mmu_write(gb, gb->cpu.sp, (u8)(gb->cpu.pc >> 8));
    gb->cpu.sp = (u16)(gb->cpu.sp - 1u);
    mmu_write(gb, gb->cpu.sp, (u8)(gb->cpu.pc & 0xFFu));
    gb->cpu.pc = vectors[bit];
    return 20;
}
