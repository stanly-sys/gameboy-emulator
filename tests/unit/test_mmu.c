/**
 * @file test_mmu.c
 * @brief Echo RAM mirror and I/O open-bus behavior.
 */

#include "gb/gb.h"
#include "test.h"

void test_mmu(void)
{
    gb_t gb;
    gb_init(&gb);
    mmu_write(&gb, 0xC000, 0xAB);
    GB_CHECK(mmu_read(&gb, 0xC000) == 0xAB);
    GB_CHECK(mmu_read(&gb, 0xE000) == 0xAB);
    GB_CHECK(mmu_read(&gb, 0xFEA0) == 0xFF);
    gb_destroy(&gb);
}
