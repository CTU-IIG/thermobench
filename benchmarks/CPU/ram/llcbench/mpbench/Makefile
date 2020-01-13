#
# Do not touch this file...
#

SHELL		= /bin/sh

include ../user.def
include ../sys.def

MPOPTS = -i ${MP_Iterations} -e ${MP_RepeatCount} -m ${MP_Memsize} -x ${MP_Resolution}
MPIFULLRUNOPTS = $(MPIRUNOPTS)

all mpi: mpi_bench

doc:
	cd doc; $(MAKE)

test test-mpi: mpi_bench 
	@echo "Running latency with small values as a test."
	$(MPIRUNCMD) $(MPIFULLRUNOPTS) $(MPIRUNPROCS) 2 $(MPIRUNPOSTOPTS) -l -i 1 -e 1 -x 0 -m 16

results:
	-ln -s ../results results

run run-mpi: results latency roundtrip bandwidth bibw reduce broadcast allreduce alltoall script

script:
	@sh make_templates.sh results/${Result_Prefix}_mpi
	@echo ""; echo "Datafiles and GNUplot scripts are located in the results directory."; echo ""
	@cd results; tar cf ${Result_Prefix}-mpbench-datafiles.tar ${Result_Prefix}_mpi*.dat ${Result_Prefix}_mpi*.gp

graphs graph: 
	sh make_graphs.sh results/${Result_Prefix}_mpi
	@echo ""; echo "MPI performance graphs are located in the results directory."; echo ""
	@cd results; tar cf ${Result_Prefix}-mpbench-graphs.tar ${Result_Prefix}_mpi*.ps

latency: mpi
	@echo "Measuring Latency..."
	$(MPIRUNCMD) $(MPIRUNOPTS) $(MPIRUNPROCS) ${MP_Tasks} $(MPIRUNPOSTOPTS) $(MPOPTS) -l > results/${Result_Prefix}_mpi_$@.dat

roundtrip: mpi
	@echo "Measuring Roundtrip..."
	$(MPIRUNCMD) $(MPIRUNOPTS) $(MPIRUNPROCS) ${MP_Tasks} $(MPIRUNPOSTOPTS) $(MPOPTS) -r > results/${Result_Prefix}_mpi_$@.dat

bandwidth: mpi
	@echo "Measuring Bandwidth..."
	$(MPIRUNCMD) $(MPIRUNOPTS) $(MPIRUNPROCS) ${MP_Tasks} $(MPIRUNPOSTOPTS) $(MPOPTS) -b > results/${Result_Prefix}_mpi_$@.dat

alltoall: mpi
	@echo "Measuring All-to-all..."
	$(MPIRUNCMD) $(MPIRUNOPTS) $(MPIRUNPROCS) ${MP_Tasks} $(MPIRUNPOSTOPTS) $(MPOPTS) -a > results/${Result_Prefix}_mpi_$@.dat

bibw: mpi
	@echo "Measuring Bidirectional Bandwidth..."
	$(MPIRUNCMD) $(MPIRUNOPTS) $(MPIRUNPROCS) ${MP_Tasks} $(MPIRUNPOSTOPTS) $(MPOPTS) -d > results/${Result_Prefix}_mpi_$@.dat

broadcast: mpi
	@echo "Measuring Broadcast..."
	$(MPIRUNCMD) $(MPIRUNOPTS) $(MPIRUNPROCS) ${MP_Tasks} $(MPIRUNPOSTOPTS) $(MPOPTS) -c > results/${Result_Prefix}_mpi_$@.dat

reduce: mpi
	@echo "Measuring Reduce..."
	$(MPIRUNCMD) $(MPIRUNOPTS) $(MPIRUNPROCS) ${MP_Tasks} $(MPIRUNPOSTOPTS) $(MPOPTS) -y > results/${Result_Prefix}_mpi_$@.dat

allreduce: mpi
	@echo "Measuring Allreduce..."
	$(MPIRUNCMD) $(MPIRUNOPTS) $(MPIRUNPROCS) ${MP_Tasks} $(MPIRUNPOSTOPTS) $(MPOPTS) -z > results/${Result_Prefix}_mpi_$@.dat

mpi_bench: mpbench.o flushall.o timer.o
	$(MP_MPI_CC) $(MP_LDFLAGS) mpbench.o flushall.o timer.o -o $@ $(MPI_LIBS) $(MP_LIBS)

mpbench.o: mpbench.c
	$(MP_MPI_CC) $(MP_CFLAGS) $(MPI_INC) -DMPI -I.. -c mpbench.c -o $@

timer.o: ../timer.c
	$(MP_MPI_CC) -c -I.. ../timer.c -o $@

flushall.o: ../flushall.c
	$(MP_MPI_CC) -c -I.. ../flushall.c -o $@

clean:
	-rm -f PI* *.o *~ core */*~

clobber: clean
	-rm -f mpi_bench
