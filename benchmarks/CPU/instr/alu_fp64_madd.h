#include "bench.h"

#define bench_t double 

int bench_func(){
    register bench_t a asm("v1");
    register bench_t b asm("v2");
    register bench_t c asm("v3");
    register bench_t d asm("v4");
    a=3;
    b=5;
    c=6;
    // 1024 instructions
    asm volatile (
        REPEAT1024("fmadd %d[d], %d[a], %d[b], %d[c]\n\t")
    : [d] "=w" (d)
    : [a] "w" (a), [b] "w" (b), [c] "w" (c) 
    :
    );

    return 1024; // return the number of instructions executed
}
