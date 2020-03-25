#include "bench.h"

#define bench_t int32_t

int bench_func()
{
    register bench_t a asm("w1");
    register bench_t b asm("w2");
    register bench_t c asm("w3");
    a = 3;
    b = 5;
    // 1024 instructions
    asm volatile(REPEAT1024("mul %w[c], %w[a], %w[b]\n\t")
                 : [ c ] "=r"(c)
                 : [ a ] "r"(a), [ b ] "r"(b)
                 :);

    return 1024; // return the number of instructions executed
}
