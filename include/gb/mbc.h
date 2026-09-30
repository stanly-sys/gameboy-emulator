/**
 * @file mbc.h
 * @brief Memory Bank Controller dispatch (none, MBC1/2/3/5).
 *
 * Hardware reference: mapper registers in 0000-7FFF / A000-BFFF.
 * Cycle model: none.
 * Thread-safety: not thread-safe.
 * Error handling: bank indices clamped to actual ROM/RAM size.
 */

#pragma once

#include "gb/types.h"

struct gb;

typedef enum {
    GB_MBC_NONE = 0,
    GB_MBC_MBC1,
    GB_MBC_MBC2,
    GB_MBC_MBC3,
    GB_MBC_MBC5,
    GB_MBC_UNSUPPORTED
} gb_mbc_kind;

typedef struct {
    gb_mbc_kind kind;
    u16 rom_bank;  /**< Bank mapped at 4000-7FFF (1..N). */
    u8 ram_bank;
    bool ram_enable;
    u8 mbc1_mode;  /**< 0 = 16/8, 1 = 4/32. */
    u8 mbc1_bank2; /**< Upper bank bits. */
    u8 rtc[5];     /**< MBC3 RTC latched snapshot (S,M,H,DL,DH). */
    bool rtc_latch;
    u8 rumble;     /**< MBC5 rumble bit (ignored on desktop). */
} mbc_t;

void mbc_init_from_type(struct gb *gb, u8 cart_type);
u8 mbc_read(struct gb *gb, u16 addr);
void mbc_write(struct gb *gb, u16 addr, u8 value);

u8 mbc_none_read(struct gb *gb, u16 addr);
void mbc_none_write(struct gb *gb, u16 addr, u8 value);
u8 mbc1_read(struct gb *gb, u16 addr);
void mbc1_write(struct gb *gb, u16 addr, u8 value);
u8 mbc2_read(struct gb *gb, u16 addr);
void mbc2_write(struct gb *gb, u16 addr, u8 value);
u8 mbc3_read(struct gb *gb, u16 addr);
void mbc3_write(struct gb *gb, u16 addr, u8 value);
u8 mbc5_read(struct gb *gb, u16 addr);
void mbc5_write(struct gb *gb, u16 addr, u8 value);

size_t mbc_rom_offset(const struct gb *gb, u16 addr);
size_t mbc_ram_offset(const struct gb *gb, u16 addr);
