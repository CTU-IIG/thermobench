#include <stdint.h>
#include <cuda.h>
#include "cuda-gpu-init.h"
#include "bench.h"
#include "cuda_fp16.h"
#include "stdio.h"

int num_sm = -1;            // Num of SMs in device
const uint64_t num_t=1024;  // Num of threads per SM
const uint64_t N=(1<<20);   // Array length
   
typedef half bench_t;      // Kernel data type
bench_t *d_a,*d_b,*d_r;
bench_t init_a = 2, init_b = 3;

__global__ void kernel(bench_t *r, bench_t *a, bench_t *b, int n){
    for(int i = 0; i < n; i++)
        r[i] = a[i] + b[i];
}

static inline int bench_func(){
    if(num_sm == -1)
        gpu_init<bench_t>(&d_a, &d_b, &d_r, init_a, init_b, N, &num_sm);
    kernel<<<num_sm, num_t>>>(d_r, d_a, d_b, N);
    cudaDeviceSynchronize();
    return N;
}
