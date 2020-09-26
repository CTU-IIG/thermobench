CXX      ?= g++
CXXFLAGS   = -O2 -flto -std=c++11 -Wall -Wextra -Wpedantic -ggdb
LDFLAGS  = -lOpenCL -lboost_program_options

all: target/cl-mem target/cl-mandelbrot

target:
	mkdir -p target

target/cl-mem: target/memory.o target/common.o
	$(CXX) -o $@ $^ $(LDFLAGS)

target/cl-mandelbrot: target/mandelbrot.o target/common.o
	$(CXX) -o $@ $^ $(LDFLAGS)

target/memory.o: src/memory.cc target/kernel_memory.h
	$(CXX) $(CXXFLAGS) -I./target -o $@ -c $<

target/mandelbrot.o: src/mandelbrot.cc target/kernel_mandelbrot.h
	$(CXX) $(CXXFLAGS) -I./target -o $@ -c $<

target/common.o: src/common.cc src/common.h | target
	$(CXX) $(CXXFLAGS) -o $@ -c $<

target/kernel_%.h: src/%.cl | target
	printf 'const char* ocl_code = R"_delim_(%s)_delim_";' "$$(cat $<)" >$@

clean:
	rm -rvf target

.PHONY: all clean yocto
