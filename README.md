# Thermobench

## Prerequisites

We use the Meson build system. On Debian-based distro, it can be
installed by:

    apt install meson

Alternatively, newest `meson` version can be installed by:

	pip3 install --user meson

## Compilation

``` sh
git update-ref refs/heads/master.trac origin/master.trac
git submodule update --init
make  # this invokes Meson
```

If you want to enable all optional features (e.g. benchmarks), run:

``` sh
meson --reconfigure -Dauto_features=enabled build
make
```

### Compilation with DEmOS scheduler support

Some Thermobench benchmarks can interact with
[DEmOS](https://github.com/CTU-IIG/demos-sched). There are two ways to enable this
interaction:
- Install the `demo-sch` library to a system-wide location (run `ninja
  install` in demo-sched build directory) and the thermobench build
  system will find it. Meson output should contain:
  
      Run-time dependency demos-sch found: YES
- With newer meson versions (≥ 0.54) you can enable DEmOS support
  without system-wide installation:

  ``` sh
  export PKG_CONFIG_PATH=/path/to/demos-sched/build/meson-uninstalled
  meson --reconfigure -Ddemos-sch=enabled build
  make
  ```
### Cross compilation

To cross-compile the tool and benchmarks for the ARM64 architecture,
install the cross-compilers. On Debian/Ubuntu, the following should
work:

    sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

Then build thermobench:

	make aarch64

The resulting binary `build-aarch64/src/thermobench` can be copied to
the target ARM system.

## Usage

The simplest useful command to try is:

	cd build
	src/thermobench benchmarks/CPU/instr/read

which runs the `read` benchmark while measuring temperature from all
available thermal zones (`/sys/devices/virtual/thermal/thermal_zone*`).

To record multiple different sensors use:

	src/thermobench --sensor=/sys/devices/virtual/thermal/thermal_zone{0..3}/temp benchmarks/CPU/instr/read

Note that `{0..3}` is expanded by BASH shell. If you use a different
shell, you might need to specify `--sensor` multiple times.

You can also record what the benchmark prints to stdout:

    src/thermobench \
        --sensor=/sys/devices/virtual/thermal/thermal_zone{0..8}/temp \
        --column=CPU{0..5}_work_done \
	    benchmarks/CPU/instr/read

To pass some switches to the benchmark program, use `--`:

	src/thermobench -- benchmarks/CPU/instr/read -m1

In this example, the benchmark will execute only on CPU0.

The full command line that we typically use on the i.MX8-based testbed
is:

    thermobench --verbose --fan-cmd='ssh imx8fan@turbot' --fan-on=0.5 --time=900 \
                --wait=30 --wait-timeout=240 --sensors_file=sensors.imx8 \
                --cpu-usage --column=CPU{0..5}_work_done \
                --output=data.csv -- ./benchmark ...


## Command line reference

<!-- help start -->
```
Usage: thermobench [OPTION...] [--] COMMAND...
Runs a benchmark COMMAND and stores the values from temperature (and other)
sensors in a .csv file. 

  -c, --column=STR           Add column to CSV populated by STR=val lines from
                             COMMAND stdout
  -e, --exec=[(COL[,...])]CMD   Execute CMD (in addition to COMMAND) and store
                             its stdout in relevant CSV columns as specified by
                             COL. If COL ends with '=', e.g. 'KEY=', store the
                             rest of stdout lines starting with KEY= in column
                             KEY. If COL start with '@' values are not stored
                             immediately when received but the last value is
                             remembered and stored synchronously with other
                             sensors. Otherwise all non-matching lines will be
                             stored in column COL. If no COL is specified,
                             first word of CMD is used as COL specification.
                             Example: --exec '(amb1=,@amb2=,amb_other) ssh
                             ambient@turbot read_temp'
  -E, --exec-wait            Wait for --exec processes to finish. Do not kill
                             them (useful for testing).
  -f, --fan-cmd=CMD          Command to control the fan. The command is invoked
                             as 'CMD <speed>', where <speed> is a number
                             between 0 and 1. Zero means the fan is off, one
                             means full speed.
  -F, --fan-on[=SPEED]       Set the fan speed while running COMMAND. If SPEED
                             is not given, it defaults to '1'.
  -l, --stdout               Log COMMAND's stdout to CSV
  -n, --name=NAME            Basename of the .csv file
  -o, --output_dir=DIR       Where to create output .csv file
  -O, --output=FILE          The name of output CSV file (overrides -o and -n).
                             Hyphen (-) means standard output
  -p, --period=TIME [ms]     Period of reading the sensors
      --sched-deadline[=BUDGET%]   Use SCHED_DEADLINE to schedule periodic
                             sampling. BUDGET% specifies execution time budget
                             in percents of the period (default is 1%).
  -s, --sensors_file=FILE    Definition of sensors to use. Each line of the
                             FILE contains either SPEC as in -S or, when the
                             line starts with '!', the rest is interpreted as
                             an argument to --exec. Lines starting with '#' are
                             ignored. When no sensors are specified via -s or
                             -S, all available thermal zones are added
                             automatically.
  -S, --sensor=SPEC          Add a sensor to the list of used sensors. SPEC is
                             FILE [NAME [UNIT]]. FILE is typically something
                             like
                             /sys/devices/virtual/thermal/thermal_zone0/temp 
  -t, --time=SECONDS         Terminate the COMMAND after this time
      --unbuffered           Flush CSV to disk after every row.
  -u, --cpu-usage            Calculate and log CPU usage.
  -v, --verbose              Print progress information to stderr.
  -w, --wait=TEMP [°C]      Wait for the temperature reported by the first
                             configured sensor to be less or equal to TEMP
                             before running the COMMAND. Wait timeout is given
                             by --wait-timeout.
  -W, --wait-timeout=SECS    Timeout in seconds for cool-down waiting (default:
                             600).
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

## See also

- [Thermobench.jl](https://ctu-iig.github.io/thermobench/dev/)

## Citing

If you find the Thermobench tool useful for your research, please consider citing our original paper in your publication list as follows:

- M. Sojka, O. Benedikt, Z. Hanzálek and P. Zaykov, "Testbed for thermal and performance analysis in MPSoC systems," 2020 15th Conference on Computer Science and Information Systems (FedCSIS), Sofia, Bulgaria, 2020, pp. 683-692, doi: 10.15439/2020F174.
