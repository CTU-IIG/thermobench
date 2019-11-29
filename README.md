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

## Command line reference

<!-- help start -->
```
Usage: thermobench [OPTION...] [--] COMMAND...
Runs a benchmark COMMAND and stores the values from temperature (and other)
sensors in a .csv file. 

  -c, --column=STR           Add column to CSV populated by STR=val lines from
                             COMMAND stdout
  -f, --fan-cmd=CMD          Command to turn the fan on (CMD 1) or off (CMD 0)
  -l, --stdout               Log COMMAND stdout to CSV
  -n, --name=NAME            Basename of the .csv file
  -o, --output_dir=DIR       Where to create output .csv file
  -O, --output=FILE          The name of output CSV file (overrides -o and -n)
  -p, --period=TIME [ms]     Period of reading the sensors
  -s, --sensors_file=FILE    Definition of sensors to use. Each line of the
                             FILE contains SPEC as in -S. When no sensors are
                             specified, all available thermal zones are added
                             automatically.
  -S, --sensor=SPEC          Add a sensor to the list of used sensors. SPEC is
                             FILE [NAME [UNIT]]. FILE is typically something
                             like
                             /sys/devices/virtual/thermal/thermal_zone0/temp 
  -t, --time=SECONDS         Terminate the COMMAND after this time
  -u, --cpu-usage            Calculate and log CPU usage.
  -w, --wait=TEMP [°C]      Wait for the temperature reported by the first
                             configured sensor to be less or equal to TEMP
                             before running the COMMAND.
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

Besides reading and storing the temperatures, values reported by the benchmark
COMMAND via its stdout can be stored in the .csv file too. This must be
explicitly enabled by -c or -l options. 

Report bugs to https://github.com/CTU-IIG/thermobench/issues.
```
<!-- help end -->
