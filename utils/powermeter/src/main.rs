use std::io::prelude::*;
use std::{
    fs::File,
    io::{self, SeekFrom},
    num::ParseIntError,
    thread::sleep,
    time::{Duration, Instant},
};
extern crate clap;
use clap::Clap;

fn parse_millis(val: &str) -> Result<Duration, ParseIntError> {
    Ok(Duration::from_millis(val.parse::<u64>()?))
}

/// Reads values from a sensor file, applies first order low-pass
/// filter and prints output with lower frequency.
#[derive(Clap)]
struct Opts {
    /// File to read sensor values from
    #[clap(default_value = "/sys/bus/i2c/devices/8-0044/hwmon/hwmon2/power1_input")]
    sensor: String,

    /// Low-pass filter time constant in milliseconds
    // Time constant of our 1st order low-pass filter: We want step
    // response to be 95% of steady state after 1 second => T = 333ms
    #[clap(short, default_value = "333", value_name = "millisec", parse(try_from_str = parse_millis))]
    time_constant: Duration,

    /// Print output no faster than this value
    #[clap(short, default_value = "250", value_name = "millisec", parse(try_from_str = parse_millis))]
    print_period: Duration,

    /// Sampling period
    #[clap(short, default_value = "10", value_name = "millisec", parse(try_from_str = parse_millis))]
    sample_period: Duration,

    /// Print debug output
    #[clap(short, long = "debug")]
    debug: bool,

    /// Initial filter output. If not specified, taken from sensor value.
    #[clap(short, value_name = "val")]
    initial: Option<f64>,
}

fn read_val(f: &mut File) -> io::Result<f64> {
    let mut contents = String::new();
    f.seek(SeekFrom::Start(0))?;
    f.read_to_string(&mut contents)?;
    return Ok(f64::from(contents.trim_end().parse::<i32>().unwrap()));
}

fn main() -> io::Result<()> {
    let opts: Opts = Opts::parse();

    let alpha: f64 =
        opts.sample_period.as_secs_f64() / (opts.time_constant + opts.sample_period).as_secs_f64();

    let mut f = File::open(&opts.sensor).expect(&opts.sensor);
    let mut last_print = Instant::now();
    let mut _i: u64 = 0; // iteration counter
    let mut y: f64 = match opts.initial {
        Some(val) => val,
        None => read_val(&mut f).expect("Sensor read error"),
    };
    loop {
        let u = read_val(&mut f)?; // filter input
        y = (1.0 - alpha) * y + alpha * u; // filter output
        if opts.debug {
            println!("#{}, input: {}, output: {}", _i, u, y);
        }
        if Instant::now() > last_print + opts.print_period {
            last_print += opts.print_period;
            println!("{}", y);
        }
        _i += 1;
        sleep(opts.sample_period);
    }
}
