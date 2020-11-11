#include "bench.h"
#include <arm_neon.h>

#define bench_t int32x4_t

int bench_func()
{
    register bench_t a asm("v1") = { 1, 2, 3, 4 };
    register bench_t b asm("v2") = { 8, 9, 10, 11 };
    register bench_t c asm("v3");

    asm volatile(REPEAT1024("mul %[c].4s, %[a].4s, %[b].4s \n") : [ c ] "=w"(c) : [ a ] "w"(a), [ b ] "w"(b) :);

    return 1024 * 4;
}
