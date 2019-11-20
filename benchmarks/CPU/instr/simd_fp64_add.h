#include <arm_neon.h>
#include "bench.h"

#define bench_t float64x2_t

int bench_func(){

    register bench_t a asm("v1") = {1,2};
    register bench_t b asm("v2") = {8,9};
    register bench_t c asm("v3");

    asm (
        REPEAT1024("fadd %[c].2d, %[a].2d, %[b].2d \n")
    : [c] "=w" (c)
    : [a] "w" (a), [b] "w" (b)
    :
    );

    volatile bench_t end = c;

    return 2*1024;
}
