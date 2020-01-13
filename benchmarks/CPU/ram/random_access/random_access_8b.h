#include "bench.h"
#include "stdint.h"
#include <stdlib.h>

#define bench_t uint8_t

#define ALLOC_SIZE (1<<30) // 1GB
enum arr_size { arr_size = ALLOC_SIZE/sizeof(bench_t)};

bench_t arr[arr_size];

#define XSTR(x) STR(x)
#define STR(x) #x

#if RAND_MAX<arr_size
#pragma message "RAND_MAX is " XSTR(RAND_MAX) ", smaller than the allocated array(" XSTR(arr_size) "). Quitting"
#endif

static inline int bench_func(){
    int idx = rand() % (arr_size);
    volatile bench_t curr = arr[idx];
    bench_t val = (bench_t) rand();
    arr[idx] = val;
    return 2;
}
