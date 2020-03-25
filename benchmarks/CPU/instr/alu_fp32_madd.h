#include "bench.h"

#define bench_t float

int bench_func()
{
    register bench_t a asm("s1");
    register bench_t b asm("s2");
    register bench_t c asm("s3");
    register bench_t d asm("s4");
    a = 3;
    b = 5;
    c = 6;
    // 1024 instructions
    asm volatile(REPEAT1024("fmadd %s[d], %s[a], %s[b], %s[c]\n\t")
                 : [ d ] "=w"(d)
                 : [ a ] "w"(a), [ b ] "w"(b), [ c ] "w"(c)
                 :);

    return 1024; // return the number of instructions executed
}
