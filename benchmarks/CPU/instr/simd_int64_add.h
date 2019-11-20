#include <arm_neon.h>
#include "bench.h"

#define bench_t int64x2_t

int bench_func(){

    register bench_t a asm("v1") = {1,2};
    register bench_t b asm("v2") = {8,9};
    register bench_t c asm("v3");

    asm (
        REPEAT1024("add %[c].2d, %[a].2d, %[b].2d \n")
    : [c] "=w" (c)
    : [a] "w" (a), [b] "w" (b)
    :
    );

    volatile bench_t end = c;

    return 1024*2;
}
