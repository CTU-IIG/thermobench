#include "bench.h"
#include <arm_neon.h>

#define bench_t float64x2_t

int bench_func()
{
    register bench_t a asm("v1") = { 1, 2 };
    register bench_t b asm("v2") = { 8, 9 };
    register bench_t c asm("v3");

    asm volatile(REPEAT256("fmul v4.2d, %[a].2d, %[b].2d\n\t"
                           "fadd %[c].2d, %[c].2d, v4.2d\n\t") REPEAT256("fmul v4.2d, %[a].2d, %[b].2d\n\t"
                                                                         "fadd %[c].2d, %[c].2d, v4.2d\n\t")
                 : [ c ] "=w"(c)
                 : [ a ] "w"(a), [ b ] "w"(b)
                 :);

    return 1024 * 2;
}
