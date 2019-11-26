# Thermobench

## Compilation

To compile thermobench, run

    git submodule update --init
	make -C src
	make -C benchmarks/CPU/instr read

Note that the above command compiles only one benchmark `read`, which
is portable to all platforms.

To cross-compile the tool and benchmarks for the ARM64 architecture,
install the cross-compilers. On Debian/Ubuntu, the following should
work:

    sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

Then specify the cross-compiler to use when calling make:

	make -C src CC=aarch64-linux-gnu-gcc CXX=aarch64-linux-gnu-g++
	make -C benchmarks/CPU/instr CC=aarch64-linux-gnu-gcc

## Usage

The simplest useful command to try is:

	src/thermobench benchmarks/CPU/instr/read

which runs the `read` benchmark while measuring temperature from all
available thermal zones (`/sys/devices/virtual/thermal/thermal_zone*`).

To record multiple different sensors use:

	src/thermobench --sensor=/sys/devices/virtual/thermal/thermal_zone{0..3}/temp benchmarks/CPU/instr/read

You can also record what the benchmark prints to stdout:

    src/thermobench \
        --sensor=/sys/devices/virtual/thermal/thermal_zone{0..8}/temp \
        --column=CPU{0..5}_work_done \
	    benchmarks/CPU/instr/read

To pass some switches to the benchmark program, use `--`:

	src/thermobench -- benchmarks/CPU/instr/read -m1

In this example, the benchmark will execute only on CPU0.
