#include "common.h"
#include "kernel_memory.h"

int main() {
    cl_platform_id platform_id = 0;
    cl_device_id device_id = 0;
    cl_kernel kernel;
    cl_int status;
    int work_done = 0;
    cl_mem buffer;
    size_t global_ws = GLOBAL_WS;
    size_t local_ws = LOCAL_WS;

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
    cl_program program;
    const char *source;
    size_t source_len;
    source = ocl_code;
    source_len = strlen(ocl_code);
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

    kernel = clCreateKernel(program, KERNEL, &status);
    if (status != CL_SUCCESS || !kernel) {
        error("clCreateKernel (%d)\n", status);
    }

    buffer = check_clCreateBuffer(
        context,
        CL_MEM_READ_WRITE,
        MEMSIZE,
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
