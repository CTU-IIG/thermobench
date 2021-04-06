use std::io::prelude::*;
use std::{
    fs::File,
    io::{self, SeekFrom},
    thread::sleep,
    time::{Duration, Instant},
};

// Time constant of out 1st order low-pass filter: We want step
// response to be 95% of steady state after 1 second => T = 1/3
const TIME_CONSTANT: Duration = Duration::from_millis(333);
const SAMPLE_PERIOD: Duration = Duration::from_millis(10);
const PRINT_PERIOD: Duration = Duration::from_millis(250);

fn main() -> io::Result<()> {
    let filename = "/sys/bus/i2c/devices/8-0044/hwmon/hwmon2/power1_input";
    let alpha: f64 = SAMPLE_PERIOD.as_secs_f64() / (TIME_CONSTANT + SAMPLE_PERIOD).as_secs_f64();
    let mut contents = String::new();
    let mut f = File::open(filename).expect(filename);
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
        if now > last_print + PRINT_PERIOD {
            last_print += PRINT_PERIOD;
            //println!("{}: Input: {}, output: {}", _i, u, y);
            println!("{}", y);
        }
        _i += 1;
        sleep(SAMPLE_PERIOD);
    }
}
