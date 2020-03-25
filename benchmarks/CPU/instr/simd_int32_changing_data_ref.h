#include "bench.h"
#include <arm_neon.h>

#define bench_t int32x4_t

int bench_func()
{
    register bench_t a asm("v1") = { 0, 0, 0, 0 };
    register bench_t b asm("v2") = { 0, 0, 0, 0 };
    register bench_t c asm("v3");
    register bench_t d asm("v4") = { 0, 0, 0, 0 };

    asm volatile(REPEAT256("add %[c].4s, %[a].4s, %[b].4s \n"
                           "add %[a].4s, %[c].4s, %[d].4s \n")
                 REPEAT256("add %[c].4s, %[a].4s, %[b].4s \n"
                           "add %[a].4s, %[c].4s, %[d].4s \n")
                 : [ c ] "=w"(c), [ a ] "+w"(a)
                 : [ d ] "w"(d), [ b ] "w"(b)
                 :);

    return 1024 * 4;
}
