#include <stdint.h>
#include <stdio.h>
#include "bench.h"
#include BENCH_H

int main(){

    uint64_t gpu_work_done = 0;
    for(int i=0; i<100; ++i){
        gpu_work_done += bench_func();
        printf("GPU_work_done/Instructions executed=%lu\n",gpu_work_done);
        fflush(stdout);
    }   

    return 0;
}
