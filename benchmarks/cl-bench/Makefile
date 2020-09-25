CC      ?= gcc
CFLAGS   = -O2 -flto -std=gnu99 -Wall -Wextra -Wpedantic -ggdb
LDFLAGS  = -lOpenCL

LOCAL_WS = 32 # 64 128 256 512 1024
GLOBAL_WS = 32 # 64 128 256 512 1024 2048 4096 8192 16384

MEMSIZE = $(shell expr 8 '*' 1048576)
BLOCKSIZE = 64
KERNEL = read
REPS = 1024

HEIGHT = 256
WIDTH = 256

MEMORY_VARS = -DGLOBAL_WS=$(GLOBAL_WS) -DLOCAL_WS=$(LOCAL_WS) \
			  -DKERNEL='"$(KERNEL)"' -DREPS=$(REPS) \
			  -DMEMSIZE=$(MEMSIZE) -DBLOCKSIZE=$(BLOCKSIZE)

MANDELBROT_VARS = -DGLOBAL_WS=$(GLOBAL_WS) -DLOCAL_WS=$(LOCAL_WS) \
                  -DWIDTH=$(WIDTH) -DHEIGHT=$(HEIGHT)


all: target/cl-mem target/cl-mandelbrot

target:
	mkdir -p target

target/cl-mem: target/memory.o target/common.o
	$(CC) -o $@ $^ $(LDFLAGS)

target/cl-mandelbrot: target/mandelbrot.o target/common.o
	$(CC) -o $@ $^ $(LDFLAGS)

target/memory.o: src/memory.c target/kernel_memory.h
ifndef MEMSIZE
	$(error MEMSIZE is undefined)
endif
ifndef BLOCKSIZE
	$(error BLOCKSIZE is undefined)
endif
ifndef KERNEL
	$(error KERNEL is undefined)
endif
ifndef REPS
	$(error REPS is undefined)
endif
ifndef GLOBAL_WS
	$(error GLOBAL_WS is undefined)
endif
ifndef LOCAL_WS
	$(error LOCAL_WS is undefined)
endif
	$(CC) $(CFLAGS) $(MEMORY_VARS) -I./target -o $@ -c $<

target/mandelbrot.o: src/mandelbrot.c target/kernel_mandelbrot.h
ifndef WIDTH
	$(error WIDTH is undefined)
endif
ifndef HEIGHT
	$(error HEIGHT is undefined)
endif
ifndef GLOBAL_WS
	$(error GLOBAL_WS is undefined)
endif
ifndef LOCAL_WS
	$(error LOCAL_WS is undefined)
endif
	$(CC) $(CFLAGS) $(MANDELBROT_VARS) -I./target -o $@ -c $<

target/common.o: src/common.c src/common.h target
	$(CC) $(CFLAGS) -o $@ -c $<

target/kernel_%.h: src/%.cl target
	echo 'const char* ocl_code = R"_mrb_(' >$@
	cpp $(MEMORY_VARS) $(MANDELBROT_VARS) $< >>$@
	echo ')_mrb_";' >>$@

clean:
	rm -rvf target

.PHONY: all clean yocto
