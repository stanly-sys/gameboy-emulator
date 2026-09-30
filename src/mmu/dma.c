/**
 * @file dma.c
 * @brief OAM DMA: copy 160 bytes from XX00-XX9F to FE00-FE9F.
 *
 * Hardware reference: write FF46 starts transfer; ~160 T-cycles.
 * Cycle model: one byte per T-cycle (simplified vs M-cycle accurate bus).
 * Thread-safety: not thread-safe.
 * Error handling: source wrap uses mmu_read so Echo/HRAM behave as on bus.
 */

#include "gb/gb.h"

void dma_start(gb_t *gb, u8 page)
{
    gb->mmu.dma_src = (u16)((u16)page << 8);
    gb->mmu.dma_bytes_left = 160;
}

void dma_step(gb_t *gb, u32 t_cycles)
{
    u32 i;
    for (i = 0; i < t_cycles && gb->mmu.dma_bytes_left > 0; i++) {
        const u16 done = (u16)(160u - gb->mmu.dma_bytes_left);
        const u8 b = mmu_read(gb, (u16)(gb->mmu.dma_src + done));
        gb->mmu.oam[done] = b;
        gb->mmu.dma_bytes_left--;
    }
}
