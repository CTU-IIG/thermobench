#CC = aarch64-linux-gnu-gcc
CFLAGS= -std=gnu99 -O3 -pthread -g
LDLIBS= -lrt

BENCHMARKS=$(patsubst %.h,%,$(wildcard *_*.h))
BENCHMARKS+=read

all: $(BENCHMARKS:%.h=%)

$(BENCHMARKS): %: %.h main.c bench.h Makefile
	$(CC) $(CFLAGS) $(LDLIBS) -DBENCH_H='"$<"' main.c -o $@

clean:
	rm -f $(BENCHMARKS)
