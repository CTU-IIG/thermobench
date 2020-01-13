CC=g++
CFLAGS= -O3 
LIBS= -lOpenCL

BENCHMARKS=$(patsubst %.h,%,$(wildcard *_*.h))

all: $(BENCHMARKS:%.h=%)

$(BENCHMARKS): %: %.h main.cpp bench.h Makefile
	$(CC) $(CFLAGS) $(LIBS) -DBENCH_H='"$<"' main.cpp -o $@

clean:
	rm -f $(BENCHMARKS)
