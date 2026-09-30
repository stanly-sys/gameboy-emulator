/**
 * @file test.h
 * @brief Tiny assert harness: no third-party test framework.
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>

static int gb_tests_failed;
static int gb_tests_run;

#define GB_CHECK(cond)                                                                               \
    do {                                                                                             \
        gb_tests_run++;                                                                              \
        if (!(cond)) {                                                                               \
            gb_tests_failed++;                                                                       \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                          \
        }                                                                                            \
    } while (0)

static inline int gb_test_finish(void)
{
    printf("%d checks, %d failed\n", gb_tests_run, gb_tests_failed);
    return gb_tests_failed == 0 ? 0 : 1;
}

void test_cpu(void);
void test_mmu(void);
void test_ppu(void);
void test_mbc(void);
void test_blargg(void);
