#include "bench.h"

#define bench_t float 

int bench_func(){
    register bench_t a asm("s1");
    register bench_t b asm("s2");
    register bench_t c asm("s3");
    a=3;
    b=5;
    // 1024 instructions
    asm(
        REPEAT1024("fadd %s[c], %s[a], %s[b]\n\t")
    : [c] "=w" (c)
    : [a] "w" (a), [b] "w" (b)
    :
    );

    volatile int end = (c > 100) ? 1 : 0;
    return 1024; // return the number of instructions executed
}
