#include "common.h"
#include "kernel_memory.h"
#include <string>
#include <boost/program_options.hpp>
#include <iostream>

using namespace std;
namespace po = boost::program_options;

int main(int argc, char **argv) {
    cl_platform_id platform_id = 0;
    cl_device_id device_id = 0;
    cl_kernel kernel;
    cl_int status;
    int work_done = 0;
    cl_mem buffer;
    size_t global_ws;
    size_t local_ws;
    size_t memsize;
    size_t blocksize;
    unsigned reps;
    string kernel_name;

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("kernel",    po::value(&kernel_name)->default_value("read"))
        ("global-ws", po::value(&global_ws)->default_value(1024))
        ("local-ws",  po::value(&local_ws)->default_value(32))
        ("reps",      po::value(&reps)->default_value(1024))
        ("memsize",   po::value(&memsize)->default_value(8*1024*1024))
        ("blocksize", po::value(&blocksize)->default_value(64))
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    get_ocl_device(&platform_id, &device_id);

    // create context
    cl_context context = clCreateContext(
        NULL,
        1,
        &device_id,
        NULL,
        NULL,
        &status
    );
    if (status != CL_SUCCESS || !context) {
        error("clCreateContext (%d)\n", status);
    }

    // creating command queue associate with the context
    cl_command_queue queue = clCreateCommandQueue(
        context,
        device_id,
        0,
        &status
    );
    if (status != CL_SUCCESS || !queue) {
        error("clCreateCommandQueue (%d)\n", status);
    }

    // create program object
    string src =
        "#define BLOCKSIZE " + to_string(blocksize) + "\n"
        "#define MEMSIZE " + to_string(memsize) + "\n"
        "#define REPS " + to_string(reps) + "\n"
        + ocl_code;
    cl_program program;
    const char *source = src.c_str();
    size_t source_len = src.size();;
    program = clCreateProgramWithSource(
        context,
        1,
        &source,
        &source_len,
        &status
    );
    if (status != CL_SUCCESS || !program) {
        error("clCreateProgramWithSource (%d)\n", status);
    }

    // build program
    status = clBuildProgram(
        program,
        1,
        &device_id,
        "", // compile options
        NULL,
        NULL
    );
    if (status != CL_SUCCESS) {
        warn("OpenCL build failed (%d). Build log follows:\n", status);
        get_program_build_log(program, device_id);
        exit(1);
    }

    kernel = clCreateKernel(program, kernel_name.c_str(), &status);
    if (status != CL_SUCCESS || !kernel) {
        error("clCreateKernel (%d)\n", status);
    }

    buffer = check_clCreateBuffer(
        context,
        CL_MEM_READ_WRITE,
        memsize,
        NULL
    );

    // run
    do {
        check_clSetKernelArg(kernel, 0, &buffer);

        check_clEnqueueNDRangeKernel(
            queue,
            kernel,
            1,
            NULL,
            &global_ws,
            &local_ws,
            0,
            NULL,
            NULL
        );

        status = clFinish(queue);
        if (status != CL_SUCCESS) {
            error("clFinish (%d)\n", status);
        }

        printf("work_done=%d\n", work_done++);
        fflush(NULL);
    } while (true);

    status = clReleaseKernel(kernel);
    if (status != CL_SUCCESS) {
        error("clReleaseKernel (%d)\n", status);
    }
    status = clReleaseProgram(program);
    if (status != CL_SUCCESS) {
        error("clReleaseProgram (%d)\n", status);
    }
    status = clReleaseCommandQueue(queue);
    if (status != CL_SUCCESS) {
        error("clReleaseCommandQueue (%d)\n", status);
    }
    status = clReleaseContext(context);
    if (status != CL_SUCCESS) {
        error("clReleaseContext (%d)\n", status);
    }

    return 0;
}
