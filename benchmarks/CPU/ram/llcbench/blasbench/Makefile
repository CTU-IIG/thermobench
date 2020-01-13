SHELL	= /bin/sh

include ../user.def
include ../sys.def

BBOPTS		= -i $(BB_Iterations) -e $(BB_RepeatCount) -m $(BB_Memsize) -x $(BB_Resolution) -c -d -o
BBAXPYOPTS	= -i $(BB_Iterations) -e $(BB_RepeatCount) -m $(BB_AXPY_Memsize) -x $(BB_Resolution) -c -d -o

all: vblasbench

vblasbench: bb.o flushall.o timer.o
	$(BB_LD) $(BB_LDFLAGS) -o $@ bb.o flushall.o timer.o $(VBLASLIB) $(BB_LIBS)

blasbench: bb.o flushall.o timer.o
	-if [ -f "$(BLASLIB)" ]; then $(BB_F77) $(BB_LDFLAGS) -o $@ bb.o flushall.o timer.o $(BLASLIB) $(BB_LIBS); fi;

bb.o: bb.c
	$(BB_CC) $(BB_CFLAGS) -I.. -c bb.c

timer.o: ../timer.c
	$(BB_CC) $(BB_CFLAGS) -c -I.. ../timer.c

flushall.o: ../flushall.c
	$(BB_CC) -c -I.. ../flushall.c

blasperf: 
	@cd results; gnuplot < ${Result_Prefix}_blasref.gp > /dev/null
	-cd results; gnuplot < ${Result_Prefix}_blasref.gp > ${Result_Prefix}_blasref.ps
	@echo ""; @echo "UTK BLAS graph is in results/${Result_Prefix}_blasref.ps"; echo

vblasperf: 
	@cd results; gnuplot < ${Result_Prefix}_blas.gp > /dev/null
	-cd results; gnuplot < ${Result_Prefix}_blas.gp > ${Result_Prefix}_blas.ps
	@echo ""; echo "Vendor BLAS graph is in results/${Result_Prefix}_blas.ps"; echo ""

compare: 
	@cd results; gnuplot < ${Result_Prefix}_blascomp.gp > /dev/null
	-cd results; gnuplot < ${Result_Prefix}_blascomp.gp > ${Result_Prefix}_blascomp.ps
	echo ""; @echo "BLAS comparison graph is in results/${Result_Prefix}_blascomp.ps"; echo

graph graphs: vblasperf
	-@cd results; tar cf ${Result_Prefix}-blasbench-graphs.tar ${Result_Prefix}_blas*.ps; 

compare.gp: 
	-@sed -e "s|TITLE|Vendor vs. UTK BLAS performance of ${Result_Prefix}|g" < blasgraph.gp > results/$@; echo "plot \"${Result_Prefix}_blas_ix}.dat\" title 'daxpy' with linespoints \\" >> results/$@; for i in vdaxpy dgemv vdgemv dgemm vdgemm; do echo ", \"$$i-${Result_Prefix}.dat\" title '$$i' with linespoints \\" >> results/$@; done;

custom.gp: 
	-@sed -e "s|TITLE|UTK BLAS performance of ${Result_Prefix}|g" < blasgraph.gp > results/$@; echo "plot \"daxpy-${Result_Prefix}.dat\" title 'saxpy' with linespoints \\" >> results/$@; for i in dgemv dgemm; do echo ", \"$$i-${Result_Prefix}.dat\" title '$$i' with linespoints \\" >> results/$@; done;

results: 
	-ln -s ../results results

run: results vdaxpy vdgemv vdgemm script

script:
	-@sed -e "s|TITLE|${BB_Prefix} BLAS performance of ${Result_Prefix}|g" < blasgraph.gp > results/${Result_Prefix}_blas.gp; echo "plot \"${Result_Prefix}_blas_vdaxpy.dat\" title 'daxpy' with linespoints \\" >> results/${Result_Prefix}_blas.gp; for i in dgemv dgemm; do echo ", \"${Result_Prefix}_blas_v$$i.dat\" title '$$i' with linespoints \\" >> results/${Result_Prefix}_blas.gp; done;
	-@cd results; tar cf ${Result_Prefix}-blasbench-datafiles.tar ${Result_Prefix}_blas*.dat ${Result_Prefix}_blas.gp; 
	@echo ""; echo "Datafiles and GNUPlot scripts are located in the results directory."; echo "";

saxpy: blasbench
	-if [ -x blasbench ]; then blasbench $(BBAXPYOPTS) -vs > results/${Result_Prefix}_blas_$@.dat; fi
daxpy: blasbench
	-if [ -x blasbench ]; then blasbench  $(BBAXPYOPTS) -v > results/${Result_Prefix}_blas_$@.dat; fi
sgemv: blasbench
	-if [ -x blasbench ]; then blasbench  $(BBOPTS) -as > results/${Result_Prefix}_blas_$@.dat; fi
dgemv: blasbench
	-if [ -x blasbench ]; then blasbench  $(BBOPTS) -a > results/${Result_Prefix}_blas_$@.dat; fi
sgemm: blasbench
	-if [ -x blasbench ]; then blasbench  $(BBOPTS) -ts > results/${Result_Prefix}_blas_$@.dat; fi
dgemm: blasbench
	-if [ -x blasbench ]; then blasbench  $(BBOPTS) -t > results/${Result_Prefix}_blas_$@.dat; fi
vsaxpy: vblasbench
	@echo "Measuring SAXPY..."
	./vblasbench  $(BBAXPYOPTS) -vs > results/${Result_Prefix}_blas_$@.dat
vdaxpy: vblasbench
	@echo "Measuring DAXPY..."
	./vblasbench  $(BBAXPYOPTS) -v > results/${Result_Prefix}_blas_$@.dat
vsgemv: vblasbench
	@echo "Measuring SGEMV..."
	./vblasbench  $(BBOPTS) -as > results/${Result_Prefix}_blas_$@.dat
vdgemv: vblasbench
	@echo "Measuring DGEMV..."
	./vblasbench  $(BBOPTS) -a > results/${Result_Prefix}_blas_$@.dat
vsgemm: vblasbench
	@echo "Measuring SGEMM..."
	./vblasbench  $(BBOPTS) -ts > results/${Result_Prefix}_blas_$@.dat
vdgemm: vblasbench
	@echo "Measuring DGEMM..."
	./vblasbench  $(BBOPTS) -t > results/${Result_Prefix}_blas_$@.dat

clean:
	-rm -f *.o *~ core */*~
clobber: clean
	-rm -f blasbench vblasbench
doc:
	cd doc; $(MAKE)
