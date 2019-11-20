#include <arm_neon.h>
#include "bench.h"

#define bench_t float32x4_t

int bench_func(){

    register bench_t a asm("v1") = {1,2,3,4};
    register bench_t b asm("v2") = {8,9,10,11};
    register bench_t c asm("v3");

    asm(
        REPEAT256("fmul v4.4s, %[a].4s, %[b].4s\n\t"
                  "fadd %[c].4s, %[c].4s, v4.4s\n\t")
        REPEAT256("fmul v4.4s, %[a].4s, %[b].4s\n\t"
                  "fadd %[c].4s, %[c].4s, v4.4s\n\t")
    : [c] "=w" (c)
    : [a] "w" (a), [b] "w" (b)
    :
    );

    volatile bench_t end = c;

    return 1024*4;
}