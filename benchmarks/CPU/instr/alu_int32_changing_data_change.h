#include "bench.h"

#define bench_t int32_t 

int bench_func(){
    register bench_t a asm("w1");
    register bench_t b asm("w2");
    register bench_t c asm("w3");
    register bench_t d asm("w4");
    a=0; 
    b=-1;
    d=1;
    // 1024 instructions
    asm(
        // Not a perfect solution, LSB of operand 2 stays the same
        // But all other input and output bits change
        // ~0 =  0 + ~0
        //  0 = ~0 +  1
        REPEAT256("add %w[c], %w[a], %w[b]\n\t"
                  "add %w[a], %w[c], %w[d]\n\t")
        REPEAT256("add %w[c], %w[a], %w[b]\n\t"
                  "add %w[a], %w[c], %w[d]\n\t")
        
    : [c] "=r" (c), [a] "+r" (a)
    : [b] "r" (b), [d] "r" (d)
    :
    );

    volatile int end = (a > 100) ? 1 : 0;
    return 1024; // return the number of instructions executed
}
