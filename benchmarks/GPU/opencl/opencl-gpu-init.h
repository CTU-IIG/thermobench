#include <stdint.h>
#include <stdio.h>
#include <CL/cl.hpp>
#include <iostream>

template<typename T>
void gpu_init(cl::CommandQueue *queue, cl::Kernel *kernel, cl::Buffer *d_a, cl::Buffer *d_b, cl::Buffer *d_r, std::string kernel_code, T a_init, T b_init, uint64_t N, int* n_sm){
    //get all platforms (drivers)
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if(all_platforms.size()==0){
        std::cout<<" No platforms found. Check OpenCL installation!\n";
        exit(1);
    }
    cl:: Platform default_platform=all_platforms[0];
     //get default device of the default platform
    std::vector<cl::Device> all_devices;
    default_platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
    if(all_devices.size()==0){
        std::cout<<" No devices found. Check OpenCL installation!\n";
        exit(1);
    }   
    cl::Device default_device=all_devices[0];

    cl::Context context = cl::Context({default_device});
    cl::Program::Sources sources;

    sources.push_back({kernel_code.c_str(),kernel_code.length()});

    cl::Program program = cl::Program(context,sources);
    if(program.build({default_device})!=CL_SUCCESS){
        std::cout<<" Error building: "<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device)<<"\n";
        exit(1);
    }   

    *d_a = cl::Buffer(context,CL_MEM_READ_WRITE,sizeof(T)*N);
    *d_b = cl::Buffer(context,CL_MEM_READ_WRITE,sizeof(T)*N);
    *d_r = cl::Buffer(context,CL_MEM_READ_WRITE,sizeof(T)*N);

    *queue = cl::CommandQueue(context,default_device);

    (*queue).enqueueFillBuffer(*d_a,a_init,0,sizeof(T)*N,NULL,NULL);
    (*queue).enqueueFillBuffer(*d_b,b_init,0,sizeof(T)*N,NULL,NULL);

    *kernel=cl::Kernel(program,"kernel_func");
    (*kernel).setArg(0,*d_a);
    (*kernel).setArg(1,*d_b);
    (*kernel).setArg(2,*d_r);

    //*n_sm = default_device.getInfo(CL_DEVICE_MAX_COMPUTE_UNITS,(long)NULL); 
    *n_sm = default_device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>(); 

}

template<typename T>
void print_buf(cl::CommandQueue queue, cl::Buffer buf, T *resbuf, const size_t buf_size){
    queue.enqueueReadBuffer(buf, CL_TRUE, 0, sizeof(T)*buf_size, resbuf);
    std::cout << "result: {";
    for (int i=0; i<buf_size; i++) {
        std::cout << resbuf[i] << " ";
    }
    std::cout << "}" << std::endl;
}


template void gpu_init<>(cl::CommandQueue *queue, cl::Kernel *kernel, cl::Buffer *d_a, cl::Buffer *d_b, cl::Buffer *d_r, std::string kernel_code, int32_t a_init, int32_t b_init, uint64_t N, int* n_sm);
template void print_buf(cl::CommandQueue queue, cl::Buffer buf, int32_t *resbuf, const size_t buf_size);
