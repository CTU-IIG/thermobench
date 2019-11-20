#include "bench.h"

static inline int bench_func()
{
    volatile int x;

    for (int i=1000; i; --i)
        x;
    return 1000;
}
