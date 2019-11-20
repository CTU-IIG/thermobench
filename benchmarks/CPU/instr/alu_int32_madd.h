#include "bench.h"

#define bench_t int32_t 

int bench_func(){
    register bench_t a asm("w1");
    register bench_t b asm("w2");
    register bench_t c asm("w3");
    register bench_t d asm("w4");
    a=3;
    b=5;
    c=5;
    // 1024 instructions
    asm(
        REPEAT1024("madd %w[d], %w[a], %w[b], %w[c]\n\t")
    : [d] "=r" (d)
    : [a] "r" (a), [b] "r" (b), [c] "r" (c)
    :
    );

    volatile int end = (d > 100) ? 1 : 0;
    return 1024; // return the number of instructions executed
}
