#include "bench.h"

#define bench_t int64_t 

int bench_func(){
    register bench_t a asm("x1");
    register bench_t b asm("x2");
    register bench_t c asm("x3");
    a=3;
    b=5;
    // 1024 instructions
    asm volatile (
        REPEAT1024("mul %[c], %[a], %[b]\n\t")
    : [c] "=r" (c)
    : [a] "r" (a), [b] "r" (b)
    :
    );

    return 1024; // return the number of instructions executed
}
