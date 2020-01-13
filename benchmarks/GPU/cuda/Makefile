CC=/usr/local/cuda/bin/nvcc
CFLAGS= -O3 -Xcompiler -fPIC -lcuda 

BENCHMARKS=$(patsubst %.h,%,$(wildcard *_*.h))

all: $(BENCHMARKS:%.h=%)

$(BENCHMARKS): %: %.h main.cpp bench.h Makefile
	$(CC) $(CFLAGS) -DBENCH_H='"$<"' -x cu main.cpp -o $@

clean:
	rm -f $(BENCHMARKS)
