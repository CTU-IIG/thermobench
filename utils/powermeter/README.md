# Powermeter sensor reading utility

Reads values from a sensor file, applies first order low-pass filter
and prints output with lower frequency.

## Compilation

    cargo build release

## Usage

    powermeter [FLAGS] [OPTIONS] [sensor]

    ARGS:
        <sensor>    File to read sensor values from [default: /sys/bus/i2c/devices/8-0044/hwmon/hwmon2/power1_input]

    FLAGS:
        -d, --debug      Print debug output
        -h, --help       Prints help information
        -V, --version    Prints version information

    OPTIONS:
        -i <val>             Initial filter output. If not specified, taken from sensor value
        -p <millisec>        Print output no faster than this value [default: 250]
        -s <millisec>        Sampling period [default: 10]
        -t <millisec>        Low-pass filter time constant in milliseconds [default: 333]

## Running on Turbot board

If you compile the program on newer system than Ubuntu 16.04 (the
system running on our turbot board), you may not be able to run the
resulting binary on turbot due to old glibc. Whether this is your case
can be determined by running `ldd` or turbot:

    $ ldd ./powermeter
    ./powermeter: /lib/x86_64-linux-gnu/libc.so.6: version `GLIBC_2.32' not found (required by ./powermeter)

The solution is to link the program statically, which can be done as follows:

    rustup target add x86_64-unknown-linux-musl
    RUSTFLAGS='-C link-arg=-s' cargo build --release --target x86_64-unknown-linux-musl

Then, the following command executes the program on turbot:

    scp ./target/x86_64-unknown-linux-musl/release/powermeter turbot: && ssh turbot ./powermeter
