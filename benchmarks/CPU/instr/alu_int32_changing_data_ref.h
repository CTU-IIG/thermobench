#include "bench.h"

#define bench_t int32_t

int bench_func()
{
    register bench_t a asm("w1");
    register bench_t b asm("w2");
    register bench_t c asm("w3");
    register bench_t d asm("w4");
    a = 0;
    b = 0;
    d = 0;
    // 1024 instructions
    asm volatile(REPEAT256("add %w[c], %w[a], %w[b]\n\t"
                           "add %w[a], %w[c], %w[d]\n\t") REPEAT256("add %w[c], %w[a], %w[b]\n\t"
                                                                    "add %w[a], %w[c], %w[d]\n\t")
                 : [ c ] "=r"(c), [ a ] "+r"(a)
                 : [ b ] "r"(b), [ d ] "r"(d)
                 :);

    return 1024; // return the number of instructions executed
}
