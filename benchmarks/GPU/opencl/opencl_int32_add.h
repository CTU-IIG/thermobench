#include <stdint.h>
#include <string>
#include <typeinfo> 
#include <CL/cl.hpp>
#include <iostream>
#include "opencl-gpu-unit.h"
#include "bench.h"

typedef int32_t bench_t;

int num_sm = -1;            // Num of SMs in device
const uint64_t num_t=1024;  // Num of threads per SM
const uint64_t N=(1<<20);   // Array length

cl::Buffer d_a, d_b, d_r;
cl::CommandQueue queue;
cl::Kernel kernel;
bench_t init_a = 2;
bench_t init_b = 3;

const std::string kernel_code=
"void kernel kernel_func(global const int* A, global const int* B, global int* C){"
"    C[get_global_id(0)] = A[get_global_id(0)] + B[get_global_id(0)];"
"}";   

static inline int bench_func(){
    printf("%s\n",kernel_code);
    exit(1);
    if(num_sm == -1)
        gpu_init<bench_t>(&queue, &kernel, &d_a, &d_b, &d_r,
                          kernel_code, init_a, init_b, N, &num_sm);

    queue.enqueueNDRangeKernel(kernel,cl::NullRange,cl::NDRange(num_t*num_sm),cl::NDRange(num_t));
    queue.finish();
    return N;
}
