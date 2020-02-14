#include "bench.h"

static inline int bench_func()
{
    volatile int x = 0;

    for (int i=1000; i; --i)
        x;
    return 1000;
}
