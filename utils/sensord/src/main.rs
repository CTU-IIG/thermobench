use clap::Parser;
use hyper::{
    header::CONTENT_TYPE,
    service::{make_service_fn, service_fn},
    Body, Request, Response, Server,
};
use std::{
    fs::File,
    io::{self, SeekFrom},
    num::ParseIntError,
    time::{Duration, Instant},
};
use std::{io::prelude::*, sync::Arc};
use std::{net::Ipv4Addr, sync::Mutex};
use tokio::io::AsyncWriteExt;
use tokio::time::interval;

fn parse_millis(val: &str) -> Result<Duration, ParseIntError> {
    Ok(Duration::from_millis(val.parse::<u64>()?))
}

/// Reads values from various sensors and publish them for Thermobench
/// and Prometheus.
#[derive(Parser)]
struct Opts {
    /// Sampling period
    #[clap(short, default_value = "10", value_name = "millisec", parse(try_from_str = parse_millis))]
    sample_period: Duration,
}

fn read_val(f: &mut File) -> io::Result<f64> {
    let mut contents = String::new();
    f.seek(SeekFrom::Start(0))?;
    f.read_to_string(&mut contents)?;
    return Ok(f64::from(contents.trim_end().parse::<i32>().unwrap()));
}

async fn serve_req(_req: Request<Body>) -> Result<Response<Body>, hyper::http::Error> {
    let encoder = prometheus::TextEncoder::new();

    let metric_families = prometheus::gather();
    let body = encoder.encode_to_string(&metric_families).unwrap();

    Response::builder()
        .status(200)
        .header(
            CONTENT_TYPE,
            format!("{}; charset=UTF-8", prometheus::TEXT_FORMAT),
        )
        .body(body.into())
}

async fn prometheus_http_server() {
    let addr = (Ipv4Addr::UNSPECIFIED, 9898).into();
    println!("Listening on http://{}", addr);

    let serve_future = Server::bind(&addr).serve(make_service_fn(|_| async {
        Ok::<_, hyper::http::Error>(service_fn(serve_req))
    }));

    if let Err(err) = serve_future.await {
        eprintln!("server error: {}", err);
    }
}

struct PowerSensor {
    file: File,
    last_power_watt: f64,
    energy: f64,
    energy_cnt: prometheus::Counter,
}

impl PowerSensor {
    fn new(name: &str, file: &str) -> PowerSensor {
        use prometheus::{labels, opts, register_counter};
        let energy_cnt = register_counter!(opts!(
            "sensor_energy",
            "Consumed energy [J]",
            labels! {"board" => name}
        ))
        .unwrap();

        PowerSensor {
            file: File::open(&file).expect(&file),
            last_power_watt: 0.0,
            energy: 0.0,
            energy_cnt,
        }
    }
    fn read(&mut self, last_read_delay: Duration) -> io::Result<()> {
        let p = read_val(&mut self.file)? / 1e6; // µW → W
        let de = p * last_read_delay.as_secs_f64();
        self.last_power_watt = p;
        self.energy += de;
        self.energy_cnt.inc_by(de);
        Ok(())
    }
}

struct TemperatureSensor {
    file_temp: File,
    file_humidity: File,
    temp: f64,
    temperature: prometheus::Gauge,
    humidity: prometheus::Gauge,
}

impl TemperatureSensor {
    fn new(name: &str, dir: &str) -> TemperatureSensor {
        use prometheus::{labels, opts, register_gauge};
        let temperature = register_gauge!(opts!(
            "sensor_temperature",
            "Temperature [°C]",
            labels! {"type" => name}
        ))
        .unwrap();

        let humidity = register_gauge!(opts!(
            "sensor_humidity",
            "Humidity [%]",
            labels! {"type" => name}
        ))
        .unwrap();

        TemperatureSensor {
            file_temp: {
                let f = format!("{}/{}", dir, "in_temp_input");
                File::open(&f).expect(&f)
            },
            file_humidity: {
                let f = format!("{}/{}", dir, "in_humidityrelative_input");
                File::open(&f).expect(&f)
            },
            temp: f64::NAN,
            temperature,
            humidity,
        }
    }
    fn read(&mut self) -> io::Result<()> {
        let t = read_val(&mut self.file_temp)? / 1e3; // → °C
        let h = read_val(&mut self.file_humidity)? / 1e3; // → %
        self.temp = t;
        self.temperature.set(t);
        self.humidity.set(h);
        Ok(())
    }
}

async fn read_power(opts: Opts, sensors: Vec<Arc<Mutex<PowerSensor>>>) {
    let mut power_interval = interval(opts.sample_period);
    let mut last_read = Instant::now();
    loop {
        power_interval.tick().await;
        let now = Instant::now();
        for s in sensors.iter() {
            s.lock().unwrap().read(now - last_read).unwrap();
        }
        last_read = now;
    }
}

async fn read_temperature(sensors: Vec<Arc<Mutex<TemperatureSensor>>>) {
    let mut temp_interval = interval(Duration::from_secs(1));
    loop {
        temp_interval.tick().await;
        for s in sensors.iter() {
            s.lock().unwrap().read().unwrap();
        }
    }
}

async fn publish_temp(
    sock: Arc<tokio::sync::Mutex<tokio::net::unix::OwnedWriteHalf>>,
    temp: Arc<Mutex<TemperatureSensor>>,
) {
    while sock
        .lock()
        .await
        .write_all(format!("ambient={}\n", temp.lock().unwrap().temp,).as_bytes())
        .await
        .is_ok()
    {
        tokio::time::sleep(Duration::from_secs(10)).await;
    }
}

async fn publish_power(
    sock: Arc<tokio::sync::Mutex<tokio::net::unix::OwnedWriteHalf>>,
    power: Arc<Mutex<PowerSensor>>,
) {
    while sock
        .lock()
        .await
        .write_all(format!("energy={:.3}\n", power.lock().unwrap().energy).as_bytes())
        .await
        .is_ok()
    {
        tokio::time::sleep(Duration::from_millis(250)).await;
    }
}

async fn unix_server(
    path: &str,
    temp: Arc<Mutex<TemperatureSensor>>,
    power: Arc<Mutex<PowerSensor>>,
) {
    use tokio::net::UnixListener;
    std::fs::remove_file(path).unwrap_or(());
    let listener = UnixListener::bind(path).unwrap();
    loop {
        if let Ok((stream, _addr)) = listener.accept().await {
            let (_, wsock) = stream.into_split();
            let sock = Arc::new(tokio::sync::Mutex::new(wsock));
            tokio::spawn(publish_temp(sock.clone(), temp.clone()));
            tokio::spawn(publish_power(sock.clone(), power.clone()));
        }
    }
}

#[tokio::main]
async fn main() -> io::Result<()> {
    let opts: Opts = Opts::parse();

    let temp_ambient = Arc::new(Mutex::new(TemperatureSensor::new(
        "ambient",
        "/sys/bus/i2c/devices/i2c-8/8-0040/iio:device0",
    )));
    let power_imx8 = Arc::new(Mutex::new(PowerSensor::new(
        "imx8",
        "/sys/bus/i2c/devices/8-0044/hwmon/hwmon2/power1_input",
    )));
    let power_imx8b = Arc::new(Mutex::new(PowerSensor::new(
        "imx8b",
        "/sys/bus/i2c/devices/8-0045/hwmon/hwmon3/power1_input",
    )));

    tokio::try_join!(
        tokio::spawn(prometheus_http_server()),
        tokio::spawn(read_temperature(vec![temp_ambient.clone()])),
        tokio::spawn(read_power(
            opts,
            vec![power_imx8.clone(), power_imx8b.clone()]
        )),
        tokio::spawn(unix_server(
            "imx8",
            temp_ambient.clone(),
            power_imx8.clone()
        )),
        tokio::spawn(unix_server(
            "imx8b",
            temp_ambient.clone(),
            power_imx8b.clone()
        )),
    )?;

    Ok(())
}
