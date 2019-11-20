# Cross-compile with:
#
#     make CC=aarch64-linux-gnu-gcc CXX=aarch64-linux-gnu-g++

CFLAGS=-g -O2 -Wall -Wno-strict-aliasing
CXXFLAGS=$(CFLAGS) -fno-exceptions

all: thermobench

thermobench: thermobench.o ev.o
	$(CXX) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	-rm -f thermobench cpu_usage *.o
