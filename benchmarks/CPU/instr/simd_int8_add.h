#include <arm_neon.h>
#include "bench.h"

#define bench_t int8x16_t

int bench_func(){

    register bench_t a asm("v1") = {0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0};
    register bench_t b asm("v2") = {8,9,10,11,12,13,14,15,15,14,13,12,11,10,9,8};
    register bench_t c asm("v3");

    asm volatile (
        REPEAT1024("add %[c].16b, %[a].16b, %[b].16b \n")
    : [c] "=w" (c)
    : [a] "w" (a), [b] "w" (b)
    :
    );


    return 1024*16;
}
