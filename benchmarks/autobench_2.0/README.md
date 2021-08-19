# Standalone autobench kernels with thermobench wrapper
Standalone autobench kernels with thermobench wrapper.

## Compiling

- Copy the content of `multibench_automotive_2.0.2653.tgz` into this
  directory such that the `single` directory is at the same level as
  `benchmarks`, `mith` and other directories from autobench).
- Reconfigure the meson build directory with:

        meson configure -Dautobench=enabled
- Build thermobench: `ninja`


## Running
All the benchmarks require files with input data to work. The path to the files is hardcoded as `../data/data_{benchmark}/{file}`. Meson creates the required symlinks during build so that the benchmarks can be run from the build directory (typically `build/benchmarks/autobench_2.0/single`). The data files can also be found in the `benchmarks/automotive/{benchmark}01/data_{benchmark}` directories. Every benchmark comes with two workload sizes: 4K and 4M. The selection of the workload size is also hardcoded: every benchmark has a "-4K" and a "-4M" binary.

By default, the benchmarks run with verification of results enabled, which limits the internal iteration count to 1 and checks the computation results. It is recommended to disable this with the `-v0` parameter. The number of iterations can be set using the `-iXXX` parameter, where `XXX` is the number of iterations. The default number of iterations is 1000. The number of iterations can also be controlled using the thermobench wrapper. However, it is important to note that every time the benchmark is finished with its internal amount of iterations (set through the `-i` parameter), it will output a run report. This can negatively impact performance if the internal iteration count is small.

List of benchmarks:
 - a2time - Angle to time conversion
 - aifirf - Finite impulse response filter
 - bitmnp - Bit manipulation
 - canrdr - CAN remote data request
 - idctrn - Inverse discrete cosine transform
 - iirflt - Infinite impulse response filter
 - matrix - Matrix arithmetic
 - pntrch - Pointer chasing
 - puwmod - Pulse width modulation
 - rspeed - Road speed calculation
 - tblook - Table lookup and interpolation
 - ttsprk - Tooth to spark

## Known issues
The following benchmarks are not built, because they do not work correctly:
 - aifftr
 - aiifft
 - basefp
 - cacheb

These benchmarks successfully compile but produce compiler warnings during the process. When executed, they crash - either they fail to load the input data, segfault, or freeze without doing anything. None of these benchmarks is part of any of the mixed workloads provided by autobench by default. The included user guide does not mention them either.