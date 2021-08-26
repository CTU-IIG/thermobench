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
    string work_done_msg;
    int max_iter;
    float escape_radius;
    uint32_t group_mask;
    uint32_t kernel_count;

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("global-ws",     po::value(&global_ws)->default_value(1024),            "global work size")
        ("local-ws",      po::value(&local_ws)->default_value(32),               "local work size")
        ("width",         po::value(&width)->default_value(1024),                "width of the resulting image - ideally multiple of global-ws")
        ("height",        po::value(&height)->default_value(16),                 "height of the resulting image")
        ("max-iter",      po::value(&max_iter)->default_value(65536),            "maximum number of iterations per pixel")
        ("escape-radius", po::value(&escape_radius)->default_value(INFINITY))
        ("group-mask",    po::value(&group_mask)->default_value(0),              "bitmask specifying which work groups should run full computation; zero means all")
        ("work-done-msg", po::value(&work_done_msg)->default_value("work_done"))
        ("kernel-count",  po::value(&kernel_count)->default_value(0),            "how many times to execute the kernel; 0 means infinity")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    if (width % global_ws != 0)
        warn("Warning: width (%d) is not multiple of global-ws (%d)\n", width, global_ws);

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
        "#define HEIGHT (" + to_string(height) + ")\n"
        "#define WIDTH (" + to_string(width) + ")\n"
        "#define MAX_ITER (" + to_string(max_iter) + ")\n"
        "#define ESCAPED(z) (" + (isinf(escape_radius) ? "false" : "(z) >= " + to_string(escape_radius*escape_radius)) + ")\n"
        "#define GROUP_MASK (" + to_string(group_mask) + ")\n"
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

        if (!work_done_msg.empty()) {
            printf("%s=%d\n", work_done_msg.c_str(), work_done++);
            fflush(NULL);
        }
    } while (kernel_count == 0 || kernel_count-- > 1);

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
