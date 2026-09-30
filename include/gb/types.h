/**
 * @file types.h
 * @brief Fixed-width aliases and portable endian helpers for the DMG core.
 *
 * Hardware reference: Sharp LR35902 / DMG-01 is little-endian in RAM.
 * Cycle model: unused here; types only.
 * Thread-safety: pure functions; no shared mutable state.
 * Error handling: none; these helpers cannot fail.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

/** @brief Store a 16-bit value in Game Boy (little-endian) byte order. */
static inline void gb_store_le16(u8 *dst, u16 value)
{
    dst[0] = (u8)(value & 0xFFu);
    dst[1] = (u8)((value >> 8) & 0xFFu);
}

/** @brief Load a 16-bit Game Boy little-endian value. */
static inline u16 gb_load_le16(const u8 *src)
{
    return (u16)((u16)src[0] | ((u16)src[1] << 8));
}

/** @brief Store little-endian 32-bit (savestate version fields). */
static inline void gb_store_le32(u8 *dst, u32 value)
{
    dst[0] = (u8)(value & 0xFFu);
    dst[1] = (u8)((value >> 8) & 0xFFu);
    dst[2] = (u8)((value >> 16) & 0xFFu);
    dst[3] = (u8)((value >> 24) & 0xFFu);
}

/** @brief Load little-endian 32-bit. */
static inline u32 gb_load_le32(const u8 *src)
{
    return (u32)src[0] | ((u32)src[1] << 8) | ((u32)src[2] << 16) | ((u32)src[3] << 24);
}

/**
 * @brief Bounded memcpy that never writes past @p dst_cap.
 * @return bytes actually copied.
 */
static inline size_t gb_memcpy_bounded(void *dst, size_t dst_cap, const void *src, size_t n)
{
    if (dst == NULL || src == NULL || dst_cap == 0u || n == 0u) {
        return 0u;
    }
    const size_t copy = (n < dst_cap) ? n : dst_cap;
    (void)memcpy(dst, src, copy);
    return copy;
}

/**
 * @brief Bounded C-string copy; always NUL-terminates if dst_cap > 0.
 */
static inline void gb_strlcpy(char *dst, size_t dst_cap, const char *src)
{
    if (dst == NULL || dst_cap == 0u) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    size_t i = 0u;
    while (i + 1u < dst_cap && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}
