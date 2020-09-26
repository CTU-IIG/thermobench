# cl-bench

OpenCl benchmarks.

Based on [`cl-mem`] benchmark. Contains a modified version of `cl-mem` for
memory-bound stress and `cl-mandelbrot` for compute bound stress.

Each benchmark has several parameters which are listed in the `Makefile`.


## Building for Yocto (outdated!)

Using temporary files from building a Yocto image.  Use `do_compile` file
from imx-gpu-sdk (others might work too).

`$BUILDIR/tmp/work/aarch64-mx8-poky-linux-imx-gpu-sdk/5.3.1-r0/temp/run.do_compile`

Extract environment setup from it (starting after `set -e` and ending before
the do_compile function).  Run `make` from a shell with this environment.


### `run.py` script

The script contains examples of building and running the benchmarks however it's
not meant to be used as is (yet).


## Building on another distribution

Requires a `opencl-headers` package (`opencl-c-headers` on Debian and
similar) and build essentials.

Compile the programs by running:

    make

### Building and running on i.MX8 with Debian

For building on i.MX8, `libOpenCL.so` from Yocto package `imx-gpu-viv`
is needed, e.g., in `/usr/local/lib`.

To run the compiled binary, other files from the same package are
needed:

- `/etc/OpenCL/vendors/Vivante.icd`
- `libVivanteOpenCL.so`
- `libGAL.so`
- `libVSC.so`
- and probably a few other libraries from the `imx-gpu-viv` package.

Just copy the relevant files from the following Yocto build directory:
`imx-yocto-bsp/imx8qmmek-build/tmp/work/aarch64-mx8-poky-linux/imx-gpu-viv/1_6.4.0.p0.0-aarch64-r0/package`.

Then run:

    ldconfig

Additionally, `galcore.ko` kernel module is needed. Copy, e.g.,
`imx-yocto-bsp/imx8qmmek-build/tmp/work/imx8qmmek-poky-linux/kernel-module-imx-gpu-viv/6.4.0.p0.0-r0/sysroot-destdir/lib/modules/4.19.35-imx_4.19.35_1.0.0+ge4452f4458e4/extra/galcore.ko`
to `lib/modules/4.19.35-imx_4.19.35_1.0.0+ge4452f4458e4/extra/` on the
board.

To run the benchmarks, run:

    LD_LIBRARY_PATH=/usr/local/lib ./target/cl-mem

## References

- [`cl-mem`]: <https://github.com/nerdralph/cl-mem>
