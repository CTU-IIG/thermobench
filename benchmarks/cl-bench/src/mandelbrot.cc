#include "common.h"
#include "kernel_mandelbrot.h"
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
    unsigned width, height;

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("global-ws", po::value(&global_ws)->default_value(1024))
        ("local-ws",  po::value(&local_ws)->default_value(32))
        ("width",     po::value(&width)->default_value(256))
        ("height",    po::value(&height)->default_value(256))
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
        "#define HEIGHT " + to_string(height) + "\n"
        "#define WIDTH " + to_string(width) + "\n"
        "#define GLOBAL_WS " + to_string(global_ws) + "\n"
        "#define LOCAL_WS " + to_string(local_ws) + "\n"
        + ocl_code;
    cl_program program;
    const char *source = src.c_str();
    size_t source_len = src.size();
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

    // Build program
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

    kernel = clCreateKernel(program, "mandelbrot", &status);
    if (status != CL_SUCCESS || !kernel) {
        error("clCreateKernel (%d)\n", status);
    }

    buffer = check_clCreateBuffer(
        context,
        CL_MEM_READ_WRITE,
        width * height,
        NULL
    );

    // Run
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
