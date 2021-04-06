use std::io::prelude::*;
use std::{
    fs::File,
    io::{self, SeekFrom},
    thread::sleep,
    time::{Duration, Instant},
};
extern crate clap;
use clap::Clap;

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
    #[clap(short, default_value = "333", value_name = "millisec")]
    time_constant: u64,

    /// Print output no faster than this value
    #[clap(short, default_value = "250", value_name = "millisec")]
    print_period: u64,

    /// Sampling period
    #[clap(short, default_value = "10", value_name = "millisec")]
    sample_period: u64,
}

fn main() -> io::Result<()> {
    let opts: Opts = Opts::parse();

    let sample_period = Duration::from_millis(opts.sample_period);
    let time_constant = Duration::from_millis(opts.time_constant);
    let print_period = Duration::from_millis(opts.print_period);
    let alpha: f64 = sample_period.as_secs_f64() / (time_constant + sample_period).as_secs_f64();

    let mut contents = String::new();
    let mut f = File::open(&opts.sensor).expect(&opts.sensor);
    let mut last_print = Instant::now();
    let mut _i: u64 = 0; // iteration counter
    let mut y: f64 = 0.0; // filter output
    loop {
        contents.clear();
        f.seek(SeekFrom::Start(0))?;
        f.read_to_string(&mut contents)?;
        let u = f64::from(contents.trim_end().parse::<i32>().unwrap()); // filter input
        y = (1.0 - alpha) * y + alpha * u; // filter output
        let now = Instant::now();
        if now > last_print + print_period {
            last_print += print_period;
            //println!("{}: Input: {}, output: {}", _i, u, y);
            println!("{}", y);
        }
        _i += 1;
        sleep(sample_period);
    }
}
