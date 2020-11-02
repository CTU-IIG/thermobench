// vim: filetype=c

#define BLOCK_CNT (GLOBAL_SIZE / LOCAL_SIZE)
#define BLOCK_SIZE (MEMSIZE / BLOCK_CNT)

#define DTYPE ulong

#if (BLOCK_SIZE / 8) % LOCAL_SIZE != 0
#error Number of elements in the block is not a multiple of local size
#endif

__kernel
void read(__global char* buffer) {
    size_t block = get_global_id(0) / LOCAL_SIZE;

    DTYPE sum = 0;
    __global DTYPE *p = (__global DTYPE*)(buffer + block * BLOCK_SIZE);

    for (uint rep = REPS; rep > 0; rep--) {
        for (uint i = 0; i < BLOCK_SIZE/sizeof(DTYPE); i += LOCAL_SIZE) {
            sum += p[i + get_local_id(0)];
        }
    }

    *p = sum;
}

__kernel
void write(__global char* buffer) {
    ulong data = get_global_id(0) << 16;
    size_t block = get_global_id(0) / LOCAL_SIZE;

    __global DTYPE *p = (__global DTYPE*)(buffer + block * BLOCK_SIZE);

    for (uint rep = REPS; rep > 0; rep--) {
        for (uint i = 0; i < BLOCK_SIZE/sizeof(DTYPE); i += LOCAL_SIZE) {
            p[i + get_local_id(0)] = data;
        }
    }
}

__kernel
void copy(__global char* buffer) {
    size_t block = get_global_id(0) / LOCAL_SIZE;

    DTYPE sum = 0;
    __global DTYPE *src = (__global DTYPE*)(buffer + block * BLOCK_SIZE);
    __global DTYPE *dst = (__global DTYPE*)(buffer + block * BLOCK_SIZE + BLOCK_SIZE/2);

    for (uint rep = REPS; rep > 0; rep--) {
        for (uint i = 0; i < BLOCK_SIZE/sizeof(DTYPE); i += LOCAL_SIZE) {
            dst[i + get_local_id(0)] = src[i + get_local_id(0)];
        }
    }
}
