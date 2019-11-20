#include "bench.h"

#define bench_t int64_t 

int bench_func(){
    register bench_t a asm("x1");
    register bench_t b asm("x2");
    register bench_t c asm("x3");
    a=3;
    b=5;
    // 1024 instructions
    asm(
        REPEAT1024("add %[c], %[a], %[b]\n\t")
    : [c] "=r" (c)
    : [a] "r" (a), [b] "r" (b)
    :
    );

    volatile int end = (c > 100) ? 1 : 0;
    return 1024; // return the number of instructions executed
}
