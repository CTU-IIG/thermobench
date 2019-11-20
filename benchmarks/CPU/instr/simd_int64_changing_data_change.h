#include <arm_neon.h>
#include "bench.h"

#define bench_t int64x2_t

int bench_func(){

    register bench_t a asm("v1") = {0,0};
    register bench_t b asm("v2") = {-1,-1};
    register bench_t c asm("v3");
    register bench_t d asm("v4") = {1,1};

    asm (
        REPEAT256("add %[c].2d, %[a].2d, %[b].2d \n"
                  "add %[a].2d, %[c].2d, %[d].2d \n")
        REPEAT256("add %[c].2d, %[a].2d, %[b].2d \n"
                  "add %[a].2d, %[c].2d, %[d].2d \n")
    : [c] "=w" (c), [a] "+w" (a)
    : [d] "w" (d), [b] "w" (b)
    :
    );

    volatile bench_t end = a;

    return 1024*2;
}
