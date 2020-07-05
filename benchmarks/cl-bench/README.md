# cl-bench

OpenCl benchmarks.

Based on [`cl-mem`] benchmark. Contains a modified version of `cl-mem` for
memory-bound stress and `cl-mandelbrot` for compute bound stress.

Each benchmark has several parameters which are listed in the `Makefile`.


## Building for Yocto

Using temporary files from building a Yocto image.  Use `do_compile` file
from imx-gpu-sdk (others might work too).

`$BUILDIR/tmp/work/aarch64-mx8-poky-linux-imx-gpu-sdk/5.3.1-r0/temp/run.do_compile`

Extract environment setup from it (starting after `set -e` and ending before
the do_compile function).  Run `make` from a shell with this environment.


### `run.py` script

The script contains examples of building and running the benchmarks however it's
not meant to be used as is (yet).


## Building on another distribution

Requires a `opencl-headers` package and build essentials.


## References

- [`cl-mem`]: <https://github.com/nerdralph/cl-mem>
