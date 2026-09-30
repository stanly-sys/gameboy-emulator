/**
 * @file mmu_io.c
 * @brief FF00-FF7F register file with hardware side effects.
 *
 * Hardware reference: JOYP, serial, timer, IF, LCD, boot latch FF50.
 * Cycle model: none beyond dispatched modules.
 * Thread-safety: not thread-safe.
 * Error handling: unknown I/O reads 0xFF.
 */

#include "gb/gb.h"

u8 mmu_read_io(gb_t *gb, u16 addr)
{
    switch (addr) {
    case 0xFF00:
        return joypad_read(gb);
    case 0xFF01:
        return gb->serial_data;
    case 0xFF02:
        return gb->serial_control;
    case 0xFF04:
    case 0xFF05:
    case 0xFF06:
    case 0xFF07:
        return timer_read(gb, addr);
    case 0xFF0F:
        return interrupts_if_read(gb);
    case 0xFF10:
    case 0xFF11:
    case 0xFF12:
    case 0xFF13:
    case 0xFF14:
    case 0xFF16:
    case 0xFF17:
    case 0xFF18:
    case 0xFF19:
    case 0xFF1A:
    case 0xFF1B:
    case 0xFF1C:
    case 0xFF1D:
    case 0xFF1E:
    case 0xFF20:
    case 0xFF21:
    case 0xFF22:
    case 0xFF23:
    case 0xFF24:
    case 0xFF25:
    case 0xFF26:
        return apu_read(gb, addr);
    case 0xFF40:
        return gb->ppu.lcdc;
    case 0xFF41:
        return ppu_stat_mode_bits(gb);
    case 0xFF42:
        return gb->ppu.scy;
    case 0xFF43:
        return gb->ppu.scx;
    case 0xFF44:
        return gb->ppu.ly;
    case 0xFF45:
        return gb->ppu.lyc;
    case 0xFF46:
        return 0xFF;
    case 0xFF47:
        return gb->ppu.bgp;
    case 0xFF48:
        return gb->ppu.obp0;
    case 0xFF49:
        return gb->ppu.obp1;
    case 0xFF4A:
        return gb->ppu.wy;
    case 0xFF4B:
        return gb->ppu.wx;
    case 0xFF50:
        return gb->mmu.boot_rom_enabled ? 0xFEu : 0xFFu;
    default:
        if (addr >= 0xFF30 && addr <= 0xFF3F) {
            return apu_read(gb, addr);
        }
        return 0xFF;
    }
}

void mmu_write_io(gb_t *gb, u16 addr, u8 value)
{
    switch (addr) {
    case 0xFF00:
        joypad_write(gb, value);
        break;
    case 0xFF01:
        gb->serial_data = value;
        break;
    case 0xFF02:
        gb->serial_control = value;
        /* Blargg: bit 7 start + internal clock dumps SB to a text buffer. */
        if ((value & 0x81u) == 0x81u) {
            if (gb->serial_len + 1u < sizeof gb->serial_out) {
                gb->serial_out[gb->serial_len++] = (char)gb->serial_data;
                gb->serial_out[gb->serial_len] = '\0';
            }
            gb->serial_control = (u8)(value & 0x7Fu);
            interrupts_request(gb, GB_INT_SERIAL);
        }
        break;
    case 0xFF04:
    case 0xFF05:
    case 0xFF06:
    case 0xFF07:
        timer_write(gb, addr, value);
        break;
    case 0xFF0F:
        interrupts_if_write(gb, value);
        break;
    case 0xFF40:
        gb->ppu.lcdc = value;
        if ((value & 0x80u) == 0) {
            gb->ppu.ly = 0;
            gb->ppu.dots = 0;
            gb->ppu.mode = GB_PPU_MODE_HBLANK;
        }
        break;
    case 0xFF41:
        gb->ppu.stat = (u8)((value & 0x78u) | (gb->ppu.stat & 0x07u));
        ppu_lyc_check(gb);
        break;
    case 0xFF42:
        gb->ppu.scy = value;
        break;
    case 0xFF43:
        gb->ppu.scx = value;
        break;
    case 0xFF44:
        /* LY is read-only on hardware; ignore to avoid crashing bad ROMs. */
        /* GB_LOG("write to LY ignored"); */
        break;
    case 0xFF45:
        gb->ppu.lyc = value;
        ppu_lyc_check(gb);
        break;
    case 0xFF46:
        dma_start(gb, value);
        break;
    case 0xFF47:
        gb->ppu.bgp = value;
        break;
    case 0xFF48:
        gb->ppu.obp0 = value;
        break;
    case 0xFF49:
        gb->ppu.obp1 = value;
        break;
    case 0xFF4A:
        gb->ppu.wy = value;
        break;
    case 0xFF4B:
        gb->ppu.wx = value;
        break;
    case 0xFF50:
        if ((value & 0x01u) != 0) {
            gb->mmu.boot_rom_enabled = false;
        }
        break;
    default:
        if (addr >= 0xFF10 && addr <= 0xFF3F) {
            apu_write(gb, addr, value);
        }
        break;
    }
}
