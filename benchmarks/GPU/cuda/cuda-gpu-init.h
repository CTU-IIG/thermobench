#include <stdint.h>
#include <stdio.h>
#include <cuda.h>
#include <thrust/fill.h>
#include <thrust/device_ptr.h>

template<typename T>
void gpu_init(T **d_a, T **d_b, T **d_r, T a_init, T b_init,uint64_t N, int* n_sm){
    cudaMalloc((void**)&(*d_a), sizeof(T) * N); 
    cudaMalloc((void**)&(*d_b), sizeof(T) * N); 
    cudaMalloc((void**)&(*d_r), sizeof(T) * N); 

    thrust::device_ptr<T> dev_ptr_a(*d_a);
    thrust::fill(dev_ptr_a, dev_ptr_a + N, a_init);
    thrust::device_ptr<T> dev_ptr_b(*d_b);
    thrust::fill(dev_ptr_b, dev_ptr_b + N, b_init);

    CUdevice dev;
    cuDeviceGet(&dev, 0);
    cuDeviceGetAttribute(n_sm, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, dev);
}

template<typename T>
void print_buf(T *buf, const size_t n){
    T C[n];
    cudaMemcpy(C, buf, n*sizeof(T), cudaMemcpyDeviceToHost);  
    std::cout << "result: {";
    for (int i=0; i<n; i++) {
        std::cout << C[i] << " ";
    }   
    std::cout << "}" << std::endl;
}


template void gpu_init<>(__half **d_a, __half **d_b, __half **d_r, __half a_init, __half b_init,uint64_t N, int* n_sm);
template void gpu_init<>(float **d_a, float **d_b, float **d_r, float a_init, float b_init,uint64_t N, int* n_sm);
template void gpu_init<>(double **d_a, double **d_b, double **d_r, double a_init, double b_init,uint64_t N, int* n_sm);
template void gpu_init<>(int16_t **d_a, int16_t **d_b, int16_t **d_r, int16_t a_init, int16_t b_init,uint64_t N, int* n_sm);
template void gpu_init<>(int32_t **d_a, int32_t **d_b, int32_t **d_r, int32_t a_init, int32_t b_init,uint64_t N, int* n_sm);
template void gpu_init<>(int64_t **d_a, int64_t **d_b, int64_t **d_r, int64_t a_init, int64_t b_init,uint64_t N, int* n_sm);

// halfs require special support, unless on ARM
template void print_buf(__half *buf, const size_t n);
template void print_buf(float *buf, const size_t n);
template void print_buf(double *buf, const size_t n);
template void print_buf(int16_t *buf, const size_t n);
template void print_buf(int32_t *buf, const size_t n);
template void print_buf(int64_t *buf, const size_t n);
