#include <arm_neon.h>
#include "bench.h"

#define bench_t int16x8_t

int bench_func(){

    register bench_t a asm("v1") = {0,1,2,3,4,5,6,7};
    register bench_t b asm("v2") = {8,9,10,11,12,13,14,15};
    register bench_t c asm("v3");

    asm (
        REPEAT1024("mul %[c].8h, %[a].8h, %[b].8h \n")
    : [c] "=w" (c)
    : [a] "w" (a), [b] "w" (b)
    :
    );

    volatile bench_t end = c;

    return 1024*8;
}
