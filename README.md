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

Command line options and their description can be printed by running

    src/thermobench --help

The simplest useful command to try is:

	src/thermobench --sensor=/sys/devices/virtual/thermal/thermal_zone0/temp benchmarks/CPU/instr/read

To record multiple sensors use (check how many thermal zones is
available on your platform):

	src/thermobench --sensor=/sys/devices/virtual/thermal/thermal_zone{0..8}/temp benchmarks/CPU/instr/read

You can also record what the benchmark prints to stdout:

    src/thermobench \
        --sensor=/sys/devices/virtual/thermal/thermal_zone{0..8}/temp \
        --column=CPU{0..5}_work_done \
	    benchmarks/CPU/instr/read
