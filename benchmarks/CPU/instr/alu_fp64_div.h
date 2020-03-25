#include "bench.h"

#define bench_t float 

int bench_func(){
    register bench_t a asm("v1");
    register bench_t b asm("v2");
    register bench_t c asm("v3");
    a=3;
    b=5;
    // 1024 instructions
    asm volatile (
        REPEAT1024("fdiv %d[c], %d[a], %d[b]\n\t")
    : [c] "=w" (c)
    : [a] "w" (a), [b] "w" (b)
    :
    );

    return 1024; // return the number of instructions executed
}
