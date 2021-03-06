#include "bench.h"

#define bench_t float

int bench_func()
{
    register bench_t a asm("v1");
    register bench_t b asm("v2");
    register bench_t c asm("v3");
    a = 3;
    b = 5;
    c = 4;
    // 1024 instructions
    asm volatile(REPEAT256("fmul d4, %d[a], %d[b]\n\t"
                           "fadd %d[c], %d[c], d4\n\t") REPEAT256("fmul d4, %d[a], %d[b]\n\t"
                                                                  "fadd %d[c], %d[c], d4\n\t")
                 : [ c ] "+w"(c)
                 : [ a ] "w"(a), [ b ] "w"(b)
                 :);

    return 1024; // return the number of instructions executed
}
