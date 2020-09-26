#include "common.h"

#ifndef NDEBUG
void debug(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}
#else
void debug(const char *fmt, ...) {
}
#endif

void warn(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

void error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}


cl_mem check_clCreateBuffer(
    cl_context   ctx,
    cl_mem_flags flags,
    size_t       size,
    void*        host_ptr
) {
    cl_int status;
    cl_mem ret = clCreateBuffer(ctx, flags, size, host_ptr, &status);
    if (status != CL_SUCCESS || ret == NULL) {
        error("clCreateBuffer (%d)\n", status);
    }
    return ret;
}

void check_clSetKernelArg(
    cl_kernel kernel,
    cl_uint   a_pos,
    cl_mem*   a
) {
    cl_int status = clSetKernelArg(kernel, a_pos, sizeof(*a), a);
    if (status != CL_SUCCESS) {
        error("clSetKernelArg (%d)\n", status);
    }
}

void check_clEnqueueNDRangeKernel(
    cl_command_queue  queue,
    cl_kernel         kernel,
    cl_uint           work_dim,
    const size_t*     global_work_offset,
    const size_t*     global_work_size,
    const size_t*     local_work_size,
    cl_uint           event_wait_list_size,
    const cl_event*   event_wait_list,
    cl_event*         event
) {
    cl_int status = clEnqueueNDRangeKernel(
        queue,
        kernel,
        work_dim,
        global_work_offset,
        global_work_size,
        local_work_size,
        event_wait_list_size,
        event_wait_list,
        event
    );
    if (status != CL_SUCCESS) {
        error("clEnqueueNDRangeKernel (%d)\n", status);
    }
}

void check_clEnqueueReadBuffer(
    cl_command_queue queue,
    cl_mem           buffer,
    cl_bool          blocking_read,
    size_t           offset,
    size_t           size,
    void*            ptr,
    cl_uint          num_events_in_wait_list,
    const cl_event*  event_wait_list,
    cl_event*        event
) {
    cl_int status = clEnqueueReadBuffer(
        queue,
        buffer,
        blocking_read,
        offset,
        size,
        ptr,
        num_events_in_wait_list,
        event_wait_list,
        event
    );
    if (status != CL_SUCCESS) {
        error("clEnqueueReadBuffer (%d)\n", status);
    }
}

void get_program_build_log(
    cl_program   program,
    cl_device_id device
) {
    char val[2*1024*1024];
    size_t ret = 0;

    cl_int status = clGetProgramBuildInfo(
        program,
        device,
        CL_PROGRAM_BUILD_LOG,
        sizeof(val), // size_t param_value_size
        &val,        // void *param_value
        &ret         // size_t *param_value_size_ret
    );
    if (status != CL_SUCCESS) {
        error("clGetProgramBuildInfo (%d)\n", status);
    }
    fprintf(stderr, "%s\n", val);
}

void print_platform_info(cl_platform_id platform) {
    char name[1024];
    size_t len = 0;
    cl_int status = clGetPlatformInfo(
        platform,
        CL_PLATFORM_NAME,
        sizeof(name),
        &name,
        &len
    );
    if (status != CL_SUCCESS) {
        error("clGetPlatformInfo (%d)\n", status);
    }
    debug("Devices on platform \"%s\":\n", name);
}

void print_device_info(unsigned i, cl_device_id d) {
    char name[1024];
    size_t len = 0;

    cl_int status = clGetDeviceInfo(d, CL_DEVICE_NAME, sizeof(name), &name, &len);
    if (status != CL_SUCCESS) {
        error("clGetDeviceInfo (%d)\n", status);
    }

    size_t work_group_size = 0;
    status = clGetDeviceInfo(d, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(work_group_size), &work_group_size, NULL);

    debug(
        "  ID %d:\n"
        "      name: %s\n"
        "      max_work_group_size: %ld\n",
        i, name, work_group_size
    );
}

static bool get_platform_device(
    cl_platform_id      platform,
    cl_platform_id*     platform_id,    // output
    cl_device_id*       device_id       // output
) {
    cl_device_type typ = CL_DEVICE_TYPE_ALL;
    cl_device_id* devices = NULL;
    cl_int status = CL_SUCCESS;
    cl_uint n_devices = 0;

    status = clGetDeviceIDs(platform, typ, 0, NULL, &n_devices);

    if (status != CL_SUCCESS) {
        error("clGetDeviceIDs (%d)\n", status);
    }

    if (n_devices == 0) {
        return false;
    }

    devices = (cl_device_id*)malloc(n_devices * sizeof(*devices));

    status = clGetDeviceIDs(platform, typ, n_devices, devices, NULL);

    if (status != CL_SUCCESS) {
        error("clGetDeviceIDs (%d)\n", status);
    }

    *platform_id = platform;
    *device_id = devices[0];

    free(devices);

    return true;
}

void get_ocl_device(
    cl_platform_id *plat_id,    // output
    cl_device_id *dev_id        // output
) {
    cl_int status = CL_SUCCESS;
    cl_platform_id* platforms = NULL;
    cl_uint n_platforms = 0;

    status = clGetPlatformIDs(0, NULL, &n_platforms);
    if (status != CL_SUCCESS) {
        error("Cannot get OpenCL platforms (%d)\n", status);
    }

    debug("Found %d OpenCL platform(s)\n", n_platforms);
    if (n_platforms == 0) {
        error("No OpenCL platforms found");
    }

    platforms = (cl_platform_id*)malloc(n_platforms * sizeof(cl_platform_id));
    if (platforms == NULL) {
        error("malloc: %s\n", strerror(errno));
    }

    status = clGetPlatformIDs(n_platforms, platforms, NULL);
    if (status != CL_SUCCESS) {
        error("clGetPlatformIDs (%d)\n", status);
    }

    cl_uint i = 0;
    for (; i < n_platforms; i++) {
        if (get_platform_device(platforms[i], plat_id, dev_id)) {
            break;
        }
    }
    free(platforms);
}
