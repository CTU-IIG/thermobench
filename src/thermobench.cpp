//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (C) 2019, 2020 Czech Technical University in Prague
//
// Authors: Tibor Rózsa <rozsatib@fel.cvut.cz>
//          Michal Sojka <michal.sojka@cvut.cz>
//
#define _POSIX_C_SOURCE 200809L
#include <argp.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <math.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include <memory>
#include <ext/stdio_filebuf.h>
#include <algorithm>
#include <signal.h>
#include "csvRow.h"

#ifdef WITH_LOCAL_LIBEV
#define EV_STANDALONE 1
#include "libev/ev++.h"
#else
#include <ev++.h>
#endif

#ifndef GIT_VERSION
#include "version.h"
#endif

using namespace std;

#define STRINGIFY(val) #val
#define TOSTRING(val) STRINGIFY(val)
#define LOC __FILE__ ":" TOSTRING(__LINE__) ": "

#define CHECK(cmd) ({ int ret = (cmd); if (ret == -1) { perror(LOC #cmd); exit(1); }; ret; })
#define CHECKPTR(cmd) ({ void *ptr = (cmd); if (ptr == (void*)-1) { perror(LOC #cmd); exit(1); }; ptr; })
#define CHECKNULL(cmd) ({ typeof(cmd) ptr = (cmd); if (ptr == NULL) { perror(LOC #cmd); exit(1); }; ptr; })
#define CHECKFGETS(s, size, stream) ({ void *ptr = fgets(s, size, stream); if (ptr == NULL) { if (feof(stream)) fprintf(stderr, LOC "fgets(" #s "): Unexpected end of stream\n"); else perror(LOC "fgets(" #s ")"); exit(1); }; ptr; })
#define CHECKTRUE(bool, msg) ({ if (!(bool)) { fprintf(stderr, "Error: " msg "\n"); exit(1); }; })

#define MAX_RESULTS 100
#define MAX_KEYS 20
#define MAX_KEY_LENGTH 50

struct sensor {
    char *path;
    char *name;
    const char *units;
    CsvColumn* column;
};

struct proc_stat_cpu {
    unsigned user, nice, system, idle,
             iowait, irq, softirq, steal,
             guest, guest_nice;
};

struct cpu_usage {
    unsigned idx;
    proc_stat_cpu current;
    proc_stat_cpu last;
    CsvColumn* column;
};

#define MAX_CPUS 256
unsigned n_cpus;
struct cpu_usage cpu_usage[MAX_CPUS];

CsvColumns columns;
CsvColumn* time_column;
CsvColumn* stdout_column;

/* Command line options */
int measure_period_ms = 100;
char *benchmark_path[2] = { NULL, NULL };
char **benchmark_argv = NULL;
double cooldown_temp = NAN;
char *fan_cmd = NULL;
char *bench_name = NULL;
const char *output_path = ".";
char *out_file = NULL;
bool write_stdout = false;
int terminate_time = 0;
bool calc_cpu_usage = false;

struct Exec {
    const string col;
    const string cmd;
    CsvColumn* column;

    Exec(const string &arg)
        // TODO: Handle errors - e.g. when arg lacks ')' (use static methods for arg parsing)
        : col(arg[0] == '(' ? arg.substr(1, arg.find_first_of(")") - 1)
                            : arg.substr(0, arg.find_first_of(" \t")))
        , cmd(arg[0] != '(' ? arg : arg.substr(arg.find_first_of(")") + 1))
    {}

    void start(ev::loop_ref loop);
    void kill();

private:
    pid_t pid = 0;
    unique_ptr<__gnu_cxx::stdio_filebuf<char>> buf;
    ev::child child;
    ev::io child_stdout;

    void child_stdout_cb(ev::io &w, int revents);
    void child_exit_cb(ev::child &w, int revents);
};

typedef struct {
    char *key;
    CsvColumn *column;
} StdoutColumn;

struct measure_state {
    struct timespec start_time;
    vector<sensor> sensors;
    FILE *out_fp;
    vector<StdoutColumn> stdoutColumns;
    vector<unique_ptr<Exec>> execs;
    pid_t child;
} state;

static string shell_quote(int argc, char **argv)
{
    string result;

    for (int i = 0; i < argc; i++) {
        bool quote = false;
        for (char *p = argv[i]; *p; p++) {
            if (isalnum(*p)
                || *p == '/'
                || *p == '.'
                || *p == ','
                || *p == '_'
                || *p == '-'
                || *p == '@'
                ) {
                continue;
            } else if (*p == '=' && i > 0) {
                continue;
            } else {
                quote = true;
                break;
            }

        }

        if (!result.empty())
            result += " ";

        if (!quote) {
            result += argv[i];
        } else {
            result += "'";
            for (char *p = argv[i]; *p; p++) {
                if (*p == '\'')
                    result += "'\\''";
                else
                    result += *p;
            }
            result += "'";
        }
    }
    return result;
}

static char *areadfileline(const char *fname)
{
    FILE *f = fopen(fname, "r");
    char *line = NULL;
    size_t len, nread;

    nread = getline(&line, &len, f);
    fclose(f);

    while (nread > 0 && isspace(line[nread - 1]))
        line[nread - 1] = 0;
    return line;
}

static void parse_sensor_spec(struct sensor *s, const char *spec)
{
    char extra;
    int ret = sscanf(spec, "%ms %ms %ms %c", &s->path, &s->name, &s->units, &extra);
    if (ret > 3)
        errx(1, "Extra text in sensor specification: %c...", extra);
    if (ret <= 2)
        s->units = NULL;
    if (ret <= 1) {
        char *p = strdup(s->path);
        char *base = basename(p);
        char *dir = dirname(p);
        char *type;
        asprintf(&type, "%s/type", dir);
        if (strcmp(base, "temp") == 0 &&
            access(type, R_OK) == 0) {
            s->name = areadfileline(type);
        } else {
            s->name = basename(dir);
        }
        free(type);
        free(p);
    }
    if (ret == 0)
        errx(1, "Invalid sensor specification: %s", spec);
}

static void read_sensor_paths(char *sensors_file)
{
    FILE *fp = fopen(sensors_file, "r");
    if (fp == NULL)
        err(1, "fopen(%s)", sensors_file);

    size_t len;
    char *line = NULL;

    while ((getline(&line, &len, fp)) != -1) {
        struct sensor sensor;
        if (line[0] == '!') {
            state.execs.emplace_back(new Exec(line + 1));
        } else {
            parse_sensor_spec(&sensor, line);
            state.sensors.push_back(sensor);
        }
    }
    if (line)
        free(line);

    fclose(fp);
}

static void add_all_thermal_zones()
{
    for (int i=0;; i++) {
        string sensor_path = "/sys/devices/virtual/thermal/thermal_zone" + to_string(i) + "/temp";
        if (access(sensor_path.c_str(), R_OK) != 0)
            break;

        struct sensor s;
        parse_sensor_spec(&s, sensor_path.c_str());
        state.sensors.push_back(s);
    }
}

static double read_sensor(char *path)
{
    double result;
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
        err(1, "Error while opening sensor file: %s", path);
    int scanret = fscanf(fp, "%lf", &result);
    if (scanret != 1) // read fail or file empty
        result = NAN;
    fclose(fp);
    return result;
}

void set_fan(char *fan_cmd, int set)
{
    string cmd(fan_cmd);
    cmd = cmd + " " + to_string(set);
    if (system(cmd.c_str()) == -1)
        err(1, "Error while executing shell command: %s\n", cmd.c_str());
}

void wait_cooldown(char *fan_cmd)
{
    if (fan_cmd)
        set_fan(fan_cmd, 1);

    while (1) {
        double temp = read_sensor(state.sensors[0].path) / 1000.0;
        fprintf(stderr, "\rCooling down to %lg, current %s temperature: %lg...",
                cooldown_temp, state.sensors[0].name, temp);
        if (temp <= cooldown_temp) {
            fprintf(stderr, "\nDone\n");
            break;
        }
        sleep(2);
    }

    if (fan_cmd)
        set_fan(fan_cmd, 0);
}

void read_procstat()
{

    for (unsigned i = 0; i < n_cpus; ++i)
        cpu_usage[i].last = cpu_usage[i].current;

    FILE *fp = fopen("/proc/stat", "r");

    // Skipping first line, as it contains the aggregate cpu data
    if(fscanf(fp, "%*[^\n]\n") == EOF)
        err(1,"fscanf /proc/stat");

    for (unsigned i = 0; i < n_cpus; i++) {
        struct proc_stat_cpu &c = cpu_usage[i].current;
        int scanret = fscanf(fp, "cpu%u %u %u %u %u %u %u %u %u %u %u\n",
                 &cpu_usage[i].idx,
                 &c.user, &c.nice, &c.system, &c.idle, &c.iowait,
                 &c.irq, &c.softirq, &c.steal, &c.guest, &c.guest_nice);
    if (scanret != 11)
        err(1,"fscanf /proc/stat");
    }

    fclose(fp);
}

// Calculate cpu usage from number of idle/non-idle cycles in /proc/stat
double get_cpu_usage(int cpu)
{

    struct proc_stat_cpu &c = cpu_usage[cpu].current;
    struct proc_stat_cpu &l = cpu_usage[cpu].last;

    // Change in idle/active cycles since last measurement
    double idle = c.idle + c.iowait - (l.idle + l.iowait);
    double active = (c.user + c.nice + c.system + c.irq +
                     c.softirq + c.steal + c.guest + c.guest_nice) -
                    (l.user + l.nice + l.system + l.irq +
                     l.softirq + l.steal + l.guest + l.guest_nice);

    return (idle+active) ? active / (active+idle) * 100 : 0;
}

void set_process_affinity(int pid, int cpu_id)
{
    cpu_set_t my_set;
    CPU_ZERO(&my_set);
    CPU_SET(cpu_id, &my_set);
    sched_setaffinity(pid, sizeof(cpu_set_t), &my_set);
}

int get_key_idx(char *key, vector<StdoutColumn> stdoutColumns)
{
    if (stdoutColumns.size() == 0)
        return -1;
    for (unsigned i = 0; i < stdoutColumns.size(); ++i){
        if (strcmp(key, stdoutColumns[i].key) == 0)
            return i;
    }
    return -1;
}

static double get_current_time()
{
    struct timespec curr_t;
    clock_gettime(CLOCK_MONOTONIC, &curr_t);

    return 1000 * (curr_t.tv_sec - state.start_time.tv_sec + (curr_t.tv_nsec - state.start_time.tv_nsec) * 1e-9);
}

static void child_stdout_cb(EV_P_ ev_io *w, int revents)
{
    FILE *workfp = fdopen(w->fd, "r");
    char buf[200];
    CsvRow row;
    while (fscanf(workfp, "%[^\n]", buf) > 0) {
        if(row.empty())
            row.set(*time_column, get_current_time());
        char *eq = strchr(buf, '=');
        if (eq) {
            *eq = 0;
            char *key = buf;
            char *value = eq + 1;
            int id = get_key_idx(key, state.stdoutColumns);

            if (id >= 0) {
                if (!row.getValue(*state.stdoutColumns[id].column).empty()) {
                    row.write(state.out_fp);
                    row.clear();
                    row.set(*time_column, get_current_time());
                }
                row.set(*state.stdoutColumns[id].column, value);
                continue;
            }
            *eq = '=';
        }
        if (write_stdout){
            row.set(*stdout_column, buf);
            row.write(state.out_fp);
            row.clear();
        }
    }

    if(!row.empty())
        row.write(state.out_fp);

    if (feof(workfp))
        ev_io_stop(EV_A_ w);
}

void Exec::start(ev::loop_ref loop) {
    int pipefds[2];

    CHECK(pipe2(pipefds, O_NONBLOCK));

    pid = CHECK(vfork());

    if (pid == 0) {
        // Child
        close(pipefds[0]);
        CHECK(dup2(CHECK(open("/dev/null", O_RDONLY)), STDIN_FILENO));
        CHECK(dup2(pipefds[1], STDOUT_FILENO));
        CHECK(execl("/bin/sh", "/bin/sh", "-c", cmd.c_str(), NULL));
    }

    close(pipefds[1]);

    child.set(loop);
    child.set<Exec, &Exec::child_exit_cb>(this);
    child.start(pid);

    child_stdout.set(loop);
    child_stdout.set<Exec, &Exec::child_stdout_cb>(this);
    child_stdout.start(pipefds[0], ev::READ);

    buf.reset(new __gnu_cxx::stdio_filebuf<char>(pipefds[0], ios::in));
}

void Exec::kill()
{
    if (pid > 0) {
        // Kill the whole process group
        ::kill(pid, SIGTERM);
    }
}

void Exec::child_stdout_cb(ev::io &w, int revents)
{
    istream pipe_in(buf.get());
    string line;

    auto it = find_if(state.execs.begin(), state.execs.end(),
                      [this](const unique_ptr<Exec> &p){ return p.get() == this; });
    int my_index = distance(state.execs.begin(), it);

    double curr_time = get_current_time();
    while (getline(pipe_in, line)) {
        line.erase(line.find_last_not_of("\r\n") + 1);
        CsvRow row;
        row.set(*time_column, curr_time);
        row.set(*state.execs[my_index]->column, line.c_str());
        row.write(state.out_fp);
    }
    if (pipe_in.eof())
        w.stop();
}

void Exec::child_exit_cb(ev::child &w, int revents)
{
    int s = w.rstatus;
    if (WIFEXITED(s) && WEXITSTATUS(s) != 0)
        fprintf(stderr, "Command '%s' exited with status %d\n",
                cmd.c_str(), WEXITSTATUS(s));
    w.stop();
    pid = 0;

}

static void child_exit_cb(EV_P_ ev_child *w, int revents)
{
    ev_child_stop(EV_A_ w);
    ev_break(EV_A_ EVBREAK_ALL);
}

static void measure_timer_cb(EV_P_ ev_timer *w, int revents)
{
    CsvRow row;
    row.set(*time_column, get_current_time());

    for (unsigned i = 0; i < state.sensors.size(); ++i){
        row.set(*state.sensors[i].column, read_sensor(state.sensors[i].path));
    }

    if (calc_cpu_usage) {
        read_procstat();
        for (unsigned i = 0; i < n_cpus; ++i){
            row.set(*cpu_usage[i].column, get_cpu_usage(i));
        }
    }

    row.write(state.out_fp);
}

static void terminate_timer_cb(EV_P_ ev_timer *w, int revents)
{
    if (state.child != 0) {
        kill(state.child, SIGTERM);
        state.child = 0;
    }
}

static void sigint_cb(struct ev_loop *loop, ev_signal *w, int revents)
{
    // Ignore SIGINT. But our child will not ignore it and exits. We
    // detect that in child_exit_cb and break the event loop.
    fprintf(stderr, "Waiting for child to terminate...\n");
    if (state.child != 0) {
        kill(state.child, SIGTERM);
        state.child = 0;
    }
}

void measure(int measure_period_ms)
{
    int p[2];
    CHECK(pipe(p));

    pid_t pid = fork();
    if (pid == -1)
        err(1, "fork");

    if (pid == 0) { // Child process - benchmark
        close(p[0]);
        dup2(p[1], STDOUT_FILENO);
        close(p[1]);
        execvp(benchmark_argv[0], benchmark_argv);
        err(1, "exec(%s)", benchmark_argv[0]);
    }

    // Parent process - measurement
    ev_io child_stdout;
    ev_timer measure_timer;
    ev_timer terminate_timer;
    ev_child child_exit;
    ev_signal signal_watcher;

    struct ev_loop *loop = EV_DEFAULT;

    ev_child_init(&child_exit, child_exit_cb, pid, 0);
    ev_child_start(loop, &child_exit);
    state.child = pid;

    CHECK(fcntl(p[0], F_SETFL, CHECK(fcntl(p[0], F_GETFL)) | O_NONBLOCK));
    close(p[1]);
    ev_io_init(&child_stdout, child_stdout_cb, p[0], EV_READ);
    ev_io_start(loop, &child_stdout);

    ev_timer_init(&measure_timer, measure_timer_cb, 0.0, measure_period_ms / 1000.0);
    ev_timer_start(loop, &measure_timer);

    if (terminate_time > 0) {
        ev_timer_init(&terminate_timer, terminate_timer_cb, terminate_time, 0);
        ev_timer_start(loop, &terminate_timer);
    }

    ev_signal_init (&signal_watcher, sigint_cb, SIGINT);
    ev_signal_start (loop, &signal_watcher);

    for (const auto &exec : state.execs)
        exec->start(loop);

    int currpriority = getpriority(PRIO_PROCESS, getpid());
    setpriority(PRIO_PROCESS, getpid(), currpriority - 1);

    clock_gettime(CLOCK_MONOTONIC, &state.start_time);

    ev_run(loop, 0);

    for (const auto &exec : state.execs)
        exec->kill();
}

static error_t parse_opt(int key, char *arg, struct argp_state *argp_state)
{
    switch (key) {
    case 'p':
        measure_period_ms = atoi(arg);
        break;
    case 'b':
        benchmark_path[0] = arg;
        benchmark_argv = benchmark_path;
        break;
    case 's':
        read_sensor_paths(arg);
        break;
    case 'S': {
        struct sensor s;
        parse_sensor_spec(&s, arg);
        state.sensors.push_back(s);
        break;
    }
    case 'w':
        cooldown_temp = atof(arg);
        break;
    case 'f':
        fan_cmd = arg;
        break;
    case 'n':
        bench_name = arg;
        break;
    case 'o':
        output_path = arg;
        break;
    case 'O':
        out_file = arg;
        break;
    case 'l':
        write_stdout = true;
        break;
    case 'c':
        StdoutColumn stdoutColumn;
        stdoutColumn.key = arg;
        state.stdoutColumns.push_back(stdoutColumn);
        break;
    case 't':
        terminate_time = atoi(arg);
        break;
    case 'u':
        calc_cpu_usage = true;
        break;
    case 'e':
        state.execs.emplace_back(new Exec(arg));
        break;
/*     case ARGP_KEY_ARG: */
/*         break; */
    case ARGP_KEY_ARGS:
        if (!benchmark_argv)
            benchmark_argv = &argp_state->argv[argp_state->next];
        else
            argp_error(argp_state, "COMMAND already specified with --benchmark");
        break;
    case ARGP_KEY_END:
        if (!benchmark_argv)
            argp_error(argp_state, "COMMAND to run was not specified");
        if (state.sensors.size() == 0)
            add_all_thermal_zones();
        if (state.sensors.size() == 0)
            argp_error(argp_state, "No sensors to measure");
        if (!bench_name)
            bench_name = basename(benchmark_argv[0]);
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/* The options we understand. */
static struct argp_option options[] = {
    { "period",         'p', "TIME [ms]",   0, "Period of reading the sensors" },
    { "measure_period", 'p', 0,             OPTION_ALIAS | OPTION_HIDDEN },
    { "benchmark",      'b', "EXECUTABLE",  OPTION_HIDDEN, "Benchmark program to execute" },
    { "benchmark_path", 'b', 0,             OPTION_ALIAS | OPTION_HIDDEN },
    { "sensors_file",   's', "FILE",        0,
      "Definition of sensors to use. Each line of the FILE contains either "
      "SPEC as in -S or, when the line starts with '!', the rest is interpreted as "
      "an argument to --exec. When no sensors are specified via -s or -S, "
      "all available thermal zones are added automatically." },
    { "sensor",         'S', "SPEC",        0,
      "Add a sensor to the list of used sensors. SPEC is FILE [NAME [UNIT]]. "
      "FILE is typically something like "
      "/sys/devices/virtual/thermal/thermal_zone0/temp " },
    { "wait",           'w', "TEMP [°C]",   0,
      "Wait for the temperature reported by the first configured sensor to be less or equal to TEMP "
      "before running the COMMAND." },
    { "fan-cmd",        'f', "CMD",         0, "Command to turn the fan on (CMD 1) or off (CMD 0)" },
    { "name",           'n', "NAME",        0, "Basename of the .csv file" },
    { "bench_name",     'n', 0,             OPTION_ALIAS | OPTION_HIDDEN },
    { "output_dir",     'o', "DIR",         0, "Where to create output .csv file" },
    { "output",         'O', "FILE",        0, "The name of output CSV file (overrides -o and -n)" },
    { "column",         'c', "STR",         0, "Add column to CSV populated by STR=val lines from COMMAND stdout" },
    { "stdout",         'l', 0,             0, "Log COMMAND stdout to CSV" },
    { "time",           't', "SECONDS",     0, "Terminate the COMMAND after this time" },
    { "cpu-usage",      'u', 0,             0, "Calculate and log CPU usage." },
    { "exec",           'e', "[(COL)]CMD",  0,
      "Execute CMD (in addition to COMMAND) and store its stdout in CSV "
      "column COL. If COL is not specified, first word of CMD is used. "
      "Example: --exec \"(ambient) ssh ambient@turbot read_temp\"" },
    { 0 }
};

const char * argp_program_bug_address = "https://github.com/CTU-IIG/thermobench/issues";
const char * argp_program_version = "thermobench " GIT_VERSION;

/* Our argp parser. */
static struct argp argp = {
    options, parse_opt, "[--] COMMAND...",

    "Runs a benchmark COMMAND and stores the values from temperature (and "
    "other) sensors in a .csv file. "

    "\v"

    "Besides reading and storing the temperatures, values reported by the "
    "benchmark COMMAND via its stdout can be stored in the .csv file too. "
    "This must be explicitly enabled by -c or -l options. "
};

string current_time()
{
    char outstr[200];
    time_t t;
    struct tm *tmp;

    t = time(NULL);
    tmp = localtime(&t);
    if (tmp == NULL) {
        perror("localtime");
        exit(EXIT_FAILURE);
    }

    if (strftime(outstr, sizeof(outstr), "%F %T %z", tmp) == 0) {
        fprintf(stderr, "strftime returned 0");
        exit(EXIT_FAILURE);
    }
    return string(outstr);
}

static void clear_sig_mask (void)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigprocmask(SIG_SETMASK, &mask, NULL);
}

void init_columns(bool write_stdout, bool calc_cpu_usage, CsvColumns &columns){
    time_column = columns.add("time/ms");

    for (unsigned int i = 0; i < state.sensors.size(); ++i)
        state.sensors[i].column = columns.add(state.sensors[i].name);

    if (calc_cpu_usage){
        char buf[100];
        for (unsigned int i = 0; i < n_cpus; ++i){
            sprintf(buf, ",CPU%u_load/%%", cpu_usage[i].idx);
            cpu_usage[i].column = columns.add(buf);
        }
    }

    for (unsigned int i = 0; i < state.stdoutColumns.size(); ++i) {
        state.stdoutColumns[i].column = (columns.add(state.stdoutColumns[i].key));
    }

    if (write_stdout)
        stdout_column = columns.add("stdout");

    for (auto &exec : state.execs) {
        exec->column = columns.add(exec->col);
    }
}

int main(int argc, char **argv)
{
    argp_parse(&argp, argc, argv, 0, 0, NULL);

    if (!isnan(cooldown_temp))
        wait_cooldown(fan_cmd);

    if (!out_file)
        asprintf(&out_file, "%s/%s.csv", output_path, bench_name);

    if (calc_cpu_usage) {
        n_cpus = sysconf(_SC_NPROCESSORS_ONLN);
        read_procstat(); // first read to initialize cpu_usage vars
    }

    state.out_fp = fopen(out_file, "w+");
    if (state.out_fp == NULL)
        err(1, "fopen(%s)", out_file);


    fprintf(state.out_fp, "# Started at: %s, Version: %s, Generated by: %s\n",
            current_time().c_str(), GIT_VERSION,
            shell_quote(argc, argv).c_str());

    init_columns(write_stdout, calc_cpu_usage, columns);
    CsvRow row;
    columns.setHeader(row);
    row.write(state.out_fp);

    // Clear signal mask in children - don't let them inherit our
    // mask, which libev "randomly" modifies
    pthread_atfork (0, 0, clear_sig_mask);

    measure(measure_period_ms);

    fclose(state.out_fp);

    fprintf(stderr, "Results stored to %s\n", out_file);

    return 0;
}
