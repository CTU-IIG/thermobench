#ifndef __COMMON_H__
#define __COMMON_H__

#define _GNU_SOURCE 1 // memrchr
#define CL_TARGET_OPENCL_VERSION 220 // OpenCL 2.2
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS // deprecated OpenCL 1.2 APIs

#include <assert.h>
#include <CL/cl.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

void debug(const char *fmt, ...);
void warn(const char *fmt, ...);
void error(const char *fmt, ...);

void get_ocl_device(cl_platform_id *plat_id, cl_device_id *dev_id);

void get_program_build_log(cl_program program, cl_device_id device);
// void print_platform_info(cl_platform_id platform);
// void print_device_info(unsigned i, cl_device_id d);

cl_mem check_clCreateBuffer(
    cl_context   ctx,
    cl_mem_flags flags,
    size_t       size,
    void*        host_ptr
);

void check_clSetKernelArg(
    cl_kernel kernel,
    cl_uint   a_pos,
    cl_mem*   a
);

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
);

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
);

#endif // __COMMON_H__
