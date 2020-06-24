// vim: filetype=c

__kernel
void read(__global char* buffer) {
    uint block = get_global_id(0) * BLOCKSIZE;

    ulong sum = 0;
    for (uint rep = REPS; rep > 0; rep--) {
        for (uint i = 0; i < BLOCKSIZE; i++) {
            sum += *(buffer + ((block + i) % MEMSIZE));
        }
    }

    *((__global ulong*)(buffer + block)) = sum;
}

__kernel
void write(__global char* buffer) {
    uint block = get_global_id(0) * BLOCKSIZE;

    ulong data = get_global_id(0) << 16;

    for (uint rep = REPS; rep > 0; rep--) {
        for (uint i = 0; i < BLOCKSIZE; i++) {
            *(buffer + ((block + i) % MEMSIZE)) = data;
        }
    }
}

__kernel
void copy(__global char* buffer) {
    uint block = get_global_id(0) * BLOCKSIZE;

    for (uint rep = REPS; rep > 0; rep--) {
        for (uint i = 0; i < (BLOCKSIZE/2); i++) {
            *(buffer + ((block + i + (BLOCKSIZE/2)) % MEMSIZE)) = *(buffer + ((block + i) % MEMSIZE));
        }
    }
}
