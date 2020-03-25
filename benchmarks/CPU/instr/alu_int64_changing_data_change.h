#include "bench.h"

#define bench_t int64_t

int bench_func()
{
    register bench_t a asm("x1");
    register bench_t b asm("x2");
    register bench_t c asm("x3");
    register bench_t d asm("x4");
    a = 0;
    b = -1;
    d = 1;
    // 1024 instructions
    asm volatile(
        // Not a perfect solution, LSB of operand 2 stays the same
        // But all other input and output bits change
        // ~0 =  0 + ~0
        //  0 = ~0 +  1
        REPEAT256("add %[c], %[a], %[b]\n\t"
                  "add %[a], %[c], %[d]\n\t")
        REPEAT256("add %[c], %[a], %[b]\n\t"
                  "add %[a], %[c], %[d]\n\t")

        : [ c ] "=r"(c), [ a ] "+r"(a)
        : [ b ] "r"(b), [ d ] "r"(d)
        :);

    return 1024; // return the number of instructions executed
}
