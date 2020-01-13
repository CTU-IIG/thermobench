SHELL = /bin/sh

include sys.def

dum: 
	@echo "Please use one of the following targets:"
	@echo
	@echo "For all three : compile, run, script, graph, clean, clobber, reconfig"
	@echo "For BlasBench : blas-bench, blas-run, blas-script, blas-graph"
	@echo "For CacheBench: cache-bench, cache-run, cache-script, cache-graph"
	@echo "For MPBench   : mp-bench, mp-run, mp-script, mp-graph"
	@echo
	@echo "clean: removes object files and leaves result files"
	@echo "clobber: removes binary files and leaves result files"
	@echo "distclean: removes everything!"

compile bench: blas-bench mp-bench cache-bench

run: blas-run cache-run mp-run

scripts script: blas-script cache-script mp-script

graphs graph: blas-graph cache-graph mp-graph

blas-bench:
	cd blasbench; make  
blas-run: 
	cd blasbench; make run
blas-script: 
	cd blasbench; make script
blas-graph:
	cd blasbench; make graph
cache-bench:
	cd cachebench; make 
cache-run:
	cd cachebench; make run
cache-script:
	cd cachebench; make script
cache-graph:
	cd cachebench; make graph
mp-bench:
	cd mpbench; make 
mp-run:
	cd mpbench; make run
mp-script:
	cd mpbench; make script
mp-graph:
	cd mpbench; make graph

sys.def reconfig:
	-rm -f sys.def
	ln -s conf/sys.default sys.def

clean:
	cd mpbench; make clean
	cd blasbench; make clean
	cd cachebench; make clean
	cd doc; make clean
	rm -f *~ */*~

clobber: 
	cd mpbench; make clobber
	cd blasbench; make clobber
	cd cachebench; make clobber
	cd doc; make clobber

distclean: clean clobber reconfig
	-rm -f *~ */*~ results/*
	rm -f mpbench/results blasbench/results cachebench/results

dist: distclean
	cd ..; rm llcbench.tar.gz; tar --exclude CVS -czvf llcbench.tar.gz llcbench
	mv ../llcbench.tar.gz .

install: 
	scp llcbench.tar.gz www/*.html ~/www-home/llcbench
	scp llcbench.tar.gz www/*.html /silk/homes/icl/projects/llcbench/

