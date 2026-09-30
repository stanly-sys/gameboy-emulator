/**
 * @file test_cpu.c
 * @brief CPU unit tests + test runner.
 */

#include "gb/gb.h"
#include "test.h"

void test_cpu(void)
{
    gb_t gb;
    gb_init(&gb);
    gb.cpu.a = 0x0F;
    gb.cpu.f = 0;
    /* Fake a NOP at 0100 after reset without ROM: PC=0100 reads unmapped cart 0xFF (RST). */
    GB_CHECK(cpu_af(&gb.cpu) == 0x01B0);
    cpu_set_flags_znhc(&gb.cpu, true, false, true, false);
    GB_CHECK(cpu_flag(&gb.cpu, GB_FLAG_Z));
    GB_CHECK(!cpu_flag(&gb.cpu, GB_FLAG_N));
    gb_destroy(&gb);
}

void test_mmu(void);
void test_ppu(void);
void test_mbc(void);
void test_blargg(void);

int main(void)
{
    test_cpu();
    test_mmu();
    test_ppu();
    test_mbc();
    test_blargg();
    return gb_test_finish();
}
