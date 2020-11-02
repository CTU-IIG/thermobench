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
#include <sstream>
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

CsvColumns columns;

struct sensor {
    string path;
    string name;
    const string units;
    const CsvColumn &column;
    sensor(const char *spec)
        : path(extractPath(spec))
        , name(extractName(spec))
        , units(extractUnits(spec))
        , column(columns.add(units.empty() ? name : name + "/" + units)) {};

private:
    static string extractPath(const string spec);
    static string extractName(const string spec);
    static string extractUnits(const string spec);
};

struct proc_stat_cpu {
    unsigned user, nice, system, idle,
             iowait, irq, softirq, steal,
             guest, guest_nice;
};

struct cpu {
    unsigned idx;
    proc_stat_cpu last;
    proc_stat_cpu current;
    const CsvColumn &column;
    cpu(unsigned idx, const struct proc_stat_cpu current) : idx(idx),
        last(current), current(current),
        column(columns.add(getHeader(idx))){};
private:
    static string getHeader(unsigned idx);
};

#define MAX_CPUS 256
unsigned n_cpus;
vector<struct cpu> cpus;

const CsvColumn &time_column = columns.add("time/ms");
const CsvColumn *stdout_column = NULL;

/* Command line options */
int measure_period_ms = 1000;
char *benchmark_path[2] = { NULL, NULL };
char **benchmark_argv = NULL;
double cooldown_temp = NAN;
int cooldown_timeout = 600;
char *fan_cmd = NULL;
float fan_on = NAN;
char *bench_name = NULL;
const char *output_path = ".";
char *out_file = NULL;
bool write_stdout = false;
int terminate_time = 0;
bool calc_cpu_usage = false;
bool exec_wait = false;
bool verbose = false;
bool verbose_needs_eol = false;

struct StdoutKeyColumn {
    const CsvColumn &column;
    const string key;
    StdoutKeyColumn(const string header, const string key) : column(columns.add(header)), key(key) {};
};

vector<string> split(const string str, const char *delimiters);

struct Exec {
    const string cmd;
    vector<StdoutKeyColumn> columns;
    const CsvColumn *stdout_col ;

    Exec(const string &arg)
        : cmd(parse_cmd(arg))
        , columns(parse_columns(arg))
        , stdout_col(find_stdout_col(columns))
    {
    }

    Exec(const Exec&) = delete;
    void operator=(const Exec&) = delete;

    void start(ev::loop_ref loop);
    void kill();

private:
    static vector<string> get_specs(const string &arg);
    static const string parse_cmd(const string &arg);
    static vector<StdoutKeyColumn> parse_columns(const string &arg);
    static const CsvColumn * find_stdout_col(const vector<StdoutKeyColumn> &keys);
    pid_t pid = 0;
    unique_ptr<__gnu_cxx::stdio_filebuf<char>> buf = nullptr;
    ev::child child = {};
    ev::io child_stdout = {};

    void child_stdout_cb(ev::io &w, int revents);
    void child_exit_cb(ev::child &w, int revents);
};

const string Exec::parse_cmd(const string &arg)
{
    string cmd;
    if (arg[0] != '(')
        cmd = arg;
    else
        cmd = arg.substr(arg.find_first_of(")") + 1);
    if (cmd.find_first_not_of(" \t\r\n") == string::npos)
        errx(1, "--exec: No command");
    return cmd;
}

vector<string> Exec::get_specs(const string &arg)
{
    size_t spec_end = arg.find_first_of(")");
    if (spec_end == string::npos)
        errx(1, "--exec: Missing ')'");

    vector<string> specs = split(arg.substr(1, spec_end - 1), ",");
    if (specs.empty())
        errx(1, "--exec: No columns specified");
    return specs;
}

vector<StdoutKeyColumn> Exec::parse_columns(const string &arg)
{
    vector<StdoutKeyColumn> keys;

    if (arg[0] == '(') {
        vector<string> specs = get_specs(arg);

        const StdoutKeyColumn *catch_all = nullptr;

        for (string spec : specs) {
            if (spec.back() == '=') {
                spec.pop_back();
                keys.push_back(StdoutKeyColumn(spec, spec));
            } else {
                if (catch_all != nullptr)
                    errx(1, "--exec: At most one COL wit '=' allowed");
                keys.push_back(StdoutKeyColumn(spec, ""));
                catch_all = &keys.back();
            }
        }
    } else {
        keys.push_back(StdoutKeyColumn(arg.substr(0, arg.find_first_of(" \t")), ""));
    }
    return keys;
}

const CsvColumn * Exec::find_stdout_col(const vector<StdoutKeyColumn> &columns)
{
    for (auto &col : columns)
        if (col.key.empty())
            return &col.column;
    return nullptr;
}

struct measure_state {
    struct timespec start_time = {0};
    vector<sensor> sensors = {};
    FILE *out_fp = nullptr;
    vector<StdoutKeyColumn> stdoutColumns = {};
    vector<unique_ptr<Exec>> execs = {};
    pid_t child = 0;
} state;

ev_timer measure_timer;
ev_timer terminate_timer;
ev_signal sigint_watcher, sigterm_watcher;

void verbose_ensure_eol()
{
    if (verbose && verbose_needs_eol) {
        fprintf(stderr, "\n");
        verbose_needs_eol = false;
    }
}

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

static string areadfileline(const char *fname)
{
    string line;
    getline(ifstream(fname, ios::in), line);
    return line;
}

static void read_sensor_paths(char *sensors_file)
{
    FILE *fp = fopen(sensors_file, "r");
    if (fp == NULL)
        err(1, "fopen(%s)", sensors_file);

    size_t len;
    char *line = NULL;

    while ((getline(&line, &len, fp)) != -1) {
        if (line[0] == '!') {
            state.execs.emplace_back(new Exec(line + 1));
        } else {
            state.sensors.push_back(sensor(line));
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

        state.sensors.push_back(sensor(sensor_path.c_str()));
    }
}

static double read_sensor(const char *path)
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

void set_fan(char *fan_cmd, float speed)
{
    char *cmd;
    CHECK(asprintf(&cmd, "%s %g", fan_cmd, speed));
    if (system(cmd) == -1)
        err(1, "Error while executing shell command: %s\n", cmd);
    free(cmd);
}

void wait_cooldown(char *fan_cmd)
{
    if (fan_cmd)
        set_fan(fan_cmd, 1);

    int time = 0;

    while (1) {
        double temp = read_sensor(state.sensors[0].path.c_str()) / 1000.0;
        fprintf(stderr, "\rCooling down to %lg°C, current %s temperature: %lg°C, time: %ds...",
                cooldown_temp, state.sensors[0].name.c_str(), temp, time);
        if (temp <= cooldown_temp) {
            fprintf(stderr, "\nDone\n");
            break;
        }
        if (time >= cooldown_timeout) {
            fprintf(stderr, "\nTimed out\n");
            break;
        }
        time += 2;
        sleep(2);
    }

    if (fan_cmd && isnan(fan_on))
        set_fan(fan_cmd, 0);
}

void read_procstat()
{
    FILE *fp = fopen("/proc/stat", "r");

    // Skipping first line, as it contains the aggregate cpu data
    if(fscanf(fp, "%*[^\n]\n") == EOF)
        err(1,"fscanf /proc/stat");

    struct proc_stat_cpu cur;
    unsigned idx;
    bool defined = cpus.size() == n_cpus;
    for (unsigned i = 0; i < n_cpus; i++) {
        if(defined)
            cpus[i].last = cpus[i].current;
        struct proc_stat_cpu &c = (defined) ? cpus[i].current : cur;
        int scanret = fscanf(fp, "cpu%u %u %u %u %u %u %u %u %u %u %u\n",
                 (defined) ? &(cpus[i].idx) : &idx,
                 &c.user, &c.nice, &c.system, &c.idle, &c.iowait,
                 &c.irq, &c.softirq, &c.steal, &c.guest, &c.guest_nice);
        if (scanret != 11)
            err(1,"fscanf /proc/stat");
        if(!defined)
            cpus.push_back(cpu(idx, c));
    }
    fclose(fp);
}

// Calculate cpu usage from number of idle/non-idle cycles in /proc/stat
double get_cpu_usage(struct cpu &cpu)
{

    struct proc_stat_cpu &c = cpu.current;
    struct proc_stat_cpu &l = cpu.last;

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

const CsvColumn *get_stdout_column(const char *key, const vector<StdoutKeyColumn> &stdoutColumns)
{
    for (unsigned i = 0; i < stdoutColumns.size(); ++i){
        if (stdoutColumns[i].key == key)
            return &(stdoutColumns[i].column);
    }
    return nullptr;
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
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    CsvRow row(columns);
    double curr_time = get_current_time();
    while ((nread = getline(&line, &len, workfp)) != -1) {
        if (nread > 0 && line[nread - 1] == '\n')
            line[nread - 1] = '\0';
        char *eq = strchr(line, '=');
        if (eq) {
            *eq = 0;
            char *key = line;
            char *value = eq + 1;
            const CsvColumn *col = get_stdout_column(key, state.stdoutColumns);

            if (col) {
                if(row.empty())
                    row.set(time_column, curr_time);
                if (!row.getValue(*col).empty()) {
                    row.write(state.out_fp);
                    row.clear();
                    row.set(time_column, curr_time);
                }
                row.set(*col, value);
                continue;
            }
            *eq = '=';
        }
        if (write_stdout){
            row.set(time_column, curr_time);
            row.set(*stdout_column, line);
            row.write(state.out_fp);
            row.clear();
        }
    }
    free(line);

    if(!row.empty())
        row.write(state.out_fp);

    // Stop the watcher if the pipe is closed. If this was the last
    // watcher, the event loop terminates.
    if (feof(workfp))
        ev_io_stop(EV_A_ w);
}

void Exec::start(ev::loop_ref loop) {
    int pipefds[2];

    CHECK(pipe2(pipefds, O_NONBLOCK));

    pid = CHECK(vfork());

    if (pid == 0) {
        // Child
        setpgid(0, 0); // Run in background process group to not receive SIGINT from terminal
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
    if (pid > 0 && !exec_wait) {
        ::kill(-pid, SIGTERM);
    }
}

void Exec::child_stdout_cb(ev::io &w, int revents)
{
    istream pipe_in(buf.get());
    string line;
    double curr_time = get_current_time();
    CsvRow row(::columns);
    while (getline(pipe_in, line)) {
        line.erase(line.find_last_not_of("\r\n") + 1);
        size_t index = line.find_first_of('=');
        const CsvColumn *column = nullptr;

        if (index != string::npos)
            column = get_stdout_column((line.substr(0, index)).c_str(), this->columns);

        if (column)
            line = line.substr(index + 1);
        else
            column = this->stdout_col;

        if (column) {
            if (row.empty())
                row.set(time_column, curr_time);

            if (!row.getValue(*column).empty()) {
                row.write(state.out_fp);
                row.clear();
                row.set(time_column, curr_time);
            }
            row.set(*column, line.c_str());
        }
    }

    if (!row.empty())
        row.write(state.out_fp);

    // Stop the watcher if the pipe is closed. If this was the last
    // watcher, the event loop terminates.
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
    // Stop all child-related watchers. If no watches remain started,
    // the event loop exits.
    ev_child_stop(EV_A_ w);

    // Stop other watchers that my block event loop from exiting.
    ev_timer_stop(EV_A_ &measure_timer);
    ev_timer_stop(EV_A_ &terminate_timer);
    ev_signal_stop(EV_A_ &sigint_watcher);
    ev_signal_stop(EV_A_ &sigterm_watcher);

    // Also kill other processes - if there are any, event loop exits
    // after all terminate.
    for (const auto &exec : state.execs)
        exec->kill();

    // Now, we wait for children stdout pipes to be closed. After all
    // are closed, our event loop exits.
}

static void measure_timer_cb(EV_P_ ev_timer *w, int revents)
{
    CsvRow row(columns);
    auto time = get_current_time();
    double temp = NAN;
    row.set(time_column, time);

    for (unsigned i = 0; i < state.sensors.size(); ++i){
        double t = read_sensor(state.sensors[i].path.c_str());
        if (isnan(temp))
            temp = t;
        row.set(state.sensors[i].column, t);
    }

    if (calc_cpu_usage) {
        read_procstat();
        for (unsigned i = 0; i < n_cpus; ++i){
            row.set(cpus[i].column, get_cpu_usage(cpus[i]));
        }
    }

    row.write(state.out_fp);

    if (verbose) {
        fprintf(stderr, "\r%.1fs  %.1f°C   ", time/1000.0, temp/1000.0);
        verbose_needs_eol = true;
    }
}

static void terminate_timer_cb(EV_P_ ev_timer *w, int revents)
{
    if (state.child != 0) {
        verbose_ensure_eol();
        fprintf(stderr, "Waiting for child to terminate...\n");
        kill(-state.child, SIGTERM);
        state.child = 0;
    }
}

// Called as a response to SIGINT and SIGTERM
static void sigint_cb(struct ev_loop *loop, ev_signal *w, int revents)
{
    verbose_ensure_eol();
    fprintf(stderr, "Waiting for child to terminate...\n");

    if (state.child != 0) {
        kill(-state.child, SIGTERM);
        state.child = 0;
    }

    // When the child terminates, we get notified via child_exit_cb.
}

void measure(int measure_period_ms)
{
    int p[2];
    CHECK(pipe(p));

    if (verbose) {
        int argc = 0;
        while (benchmark_argv[argc] != NULL)
            argc++;
        fprintf(stderr, "Running: %s\n", shell_quote(argc, benchmark_argv).c_str());
    }

    pid_t pid = fork();
    if (pid == -1)
        err(1, "fork");

    if (pid == 0) { // Child process - benchmark
        close(p[0]);
        dup2(p[1], STDOUT_FILENO);
        close(p[1]);

        // Run the benchmark in background process group so that we
        // can kill it with its all potential children.
        setpgid(0, 0);
        // Background processes are stopped when they happen to read
        // from a controlling terminal. We don't want the benchmark to
        // be stopped, so we do not run in on terminal. If stdin is
        // not a terminal, we keep it connected to the child, because
        // it can be a pipe with input data for the benchmark.
        if (isatty(STDIN_FILENO))
            CHECK(dup2(CHECK(open("/dev/null", O_RDONLY)), STDIN_FILENO));

        execvp(benchmark_argv[0], benchmark_argv);
        err(1, "exec(%s)", benchmark_argv[0]);
    }

    // Parent process - measurement
    ev_io child_stdout;
    ev_child child_exit;

    struct ev_loop *loop = EV_DEFAULT;

    // Run the loop once to update time information. This ensures that
    // all timers are relative to now and not to the start of the
    // program, where the default loop was initialized. Due to
    // cooldown waiting, the program start time can differ from now
    // significantly. There are no watchers so no callback is invoked.
    ev_run(loop, EVRUN_NOWAIT);

    ev_child_init(&child_exit, child_exit_cb, pid, 0);
    ev_child_start(loop, &child_exit);
    state.child = pid;

    CHECK(fcntl(p[0], F_SETFL, CHECK(fcntl(p[0], F_GETFL)) | O_NONBLOCK));
    close(p[1]);
    ev_io_init(&child_stdout, child_stdout_cb, p[0], EV_READ);
    ev_io_start(loop, &child_stdout);

    ev_timer_init(&measure_timer, measure_timer_cb, 0.0, measure_period_ms / 1000.0);
    if (state.sensors.size())
        ev_timer_start(loop, &measure_timer);

    if (terminate_time > 0) {
        ev_timer_init(&terminate_timer, terminate_timer_cb, terminate_time, 0);
        ev_timer_start(loop, &terminate_timer);
    }

    ev_signal_init (&sigint_watcher, sigint_cb, SIGINT);
    ev_signal_start (loop, &sigint_watcher);
    ev_signal_init (&sigterm_watcher, sigint_cb, SIGTERM);
    ev_signal_start (loop, &sigterm_watcher);

    for (const auto &exec : state.execs)
        exec->start(loop);

    int currpriority = getpriority(PRIO_PROCESS, getpid());
    setpriority(PRIO_PROCESS, getpid(), currpriority - 1);

    clock_gettime(CLOCK_MONOTONIC, &state.start_time);

    ev_run(loop, 0);

    verbose_ensure_eol();
}

static error_t parse_opt(int key, char *arg, struct argp_state *argp_state)
{
    static bool sensors_specified = false;

    switch (key) {
    case 'p':
        measure_period_ms = atoi(arg);
        break;
    case 'b':
        benchmark_path[0] = arg;
        benchmark_argv = benchmark_path;
        break;
    case 's':
        sensors_specified = true;
        read_sensor_paths(arg);
        break;
    case 'S': {
        sensors_specified = true;
        state.sensors.push_back(sensor(arg));
        break;
    }
    case 'w':
        cooldown_temp = atof(arg);
        break;
    case 'W':
        cooldown_timeout = atoi(arg);
        break;
    case 'f':
        fan_cmd = arg;
        break;
    case 'F':
        fan_on = (arg == 0) ? 1 : atof(arg);
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
        state.stdoutColumns.push_back(StdoutKeyColumn(arg, arg));
        break;
    case 't':
        terminate_time = atoi(arg);
        break;
    case 'u':
        calc_cpu_usage = true;
        break;
    case 'v':
        verbose = true;
        break;
    case 'e':
        state.execs.emplace_back(new Exec(arg));
        break;
    case 'E':
        exec_wait = true;
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
        if (!sensors_specified && state.sensors.size() == 0)
            add_all_thermal_zones();
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
      "before running the COMMAND. Wait timeout is given by --wait-timeout." },
    { "wait-timeout",   'W', "SECS",        0,
      "Timeout in seconds for cool-down waiting (default: 600)." },
    { "fan-cmd",        'f', "CMD",         0,
      "Command to control the fan. The command is invoked as 'CMD <speed>', "
      "where <speed> is a number between 0 and 1. Zero means the fan is off, one means full speed." },
    { "fan-on",         'F', "SPEED",       OPTION_ARG_OPTIONAL,
      "Set the fan speed while running COMMAND. If SPEED is not given, it defaults to '1'." },
    { "name",           'n', "NAME",        0, "Basename of the .csv file" },
    { "bench_name",     'n', 0,             OPTION_ALIAS | OPTION_HIDDEN },
    { "output_dir",     'o', "DIR",         0, "Where to create output .csv file" },
    { "output",         'O', "FILE",        0,
      "The name of output CSV file (overrides -o and -n). Hyphen (-) means standard output" },
    { "column",         'c', "STR",         0, "Add column to CSV populated by STR=val lines from COMMAND stdout" },
    { "stdout",         'l', 0,             0, "Log COMMAND's stdout to CSV" },
    { "time",           't', "SECONDS",     0, "Terminate the COMMAND after this time" },
    { "cpu-usage",      'u', 0,             0, "Calculate and log CPU usage." },
    { "exec",           'e', "[(COL[,...])]CMD",  0,

      "Execute CMD (in addition to COMMAND) and store its stdout in relevant "
      "CSV columns as specified by COL. If COL ends with '=', such as 'KEY=', "
      "store the rest of stdout lines starting with KEY= in column KEY. "
      "Otherwise all non-matching lines will be stored in column COL. If no "
      "COL is specified, first word of CMD is used as COL specification. "

      "Example: --exec '(amb1=,amb2=,amb_other) ssh ambient@turbot read_temp'"

    },
    { "exec-wait",      'E', 0,             0,
      "Wait for --exec processes to finish. Do not kill them (useful for testing)." },
    { "verbose",        'v', 0,             0, "Print progress information to stderr." },
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

vector<string> split_words(const string str)
{
    istringstream ss(str);
    vector<string> words;
    string word;

    while (ss >> word)
        words.push_back(word);
    return words;
}

vector<string> split(const string str, const char *delimiters)
{
    vector<string> words;

    for (size_t start = 0, end = 0;;) {
        start = str.find_first_not_of(delimiters, end);
        if (start == string::npos)
            break;

        end = str.find_first_of(delimiters, start);
        if (end == string::npos)
            end = str.size();

        words.push_back(str.substr(start, end - start));
    }
    return words;
}


string sensor::extractPath(const string spec)
{
    auto words = split_words(spec);

    if (words.size() < 1)
        errx(1, "Invalid sensor specification: %s", spec.c_str());
    return words[0];
}

string sensor::extractName(const string spec)
{
    string name;
    auto words = split_words(spec);

    if (words.size() < 1) {
        errx(1, "Invalid sensor specification: %s", spec.c_str());
    } else if (words.size() == 1) {
        char *p = strdup(words[0].c_str());
        char *base = basename(p);
        char *dir = dirname(p);
        char *type;
        CHECK(asprintf(&type, "%s/type", dir));
        if (strcmp(base, "temp") == 0 &&
            access(type, R_OK) == 0)
            name = areadfileline(type);
        else
            name = basename(dir);
        free(type);
        free(p);
    } else {
        name = words[1];
    }
    return name;
}

string sensor::extractUnits(const string spec)
{
    auto words = split_words(spec);
    return words.size() >= 3 ? words[2] : "";
}

string cpu::getHeader(unsigned idx)
{
    stringstream header;
    header << "CPU" << idx << "_load/%";
    return header.str();
}

int main(int argc, char **argv)
{
    argp_parse(&argp, argc, argv, 0, 0, NULL);

    if (!isnan(cooldown_temp))
        wait_cooldown(fan_cmd);

    if (!isnan(fan_on) && fan_cmd)
        set_fan(fan_cmd, fan_on);

    if (!out_file)
        CHECK(asprintf(&out_file, "%s/%s.csv", output_path, bench_name));

    if (calc_cpu_usage) {
        n_cpus = sysconf(_SC_NPROCESSORS_ONLN);
        read_procstat(); // first read to initialize cpu_usage vars
    }

    if (strcmp(out_file, "-") != 0) {
        if (verbose)
            fprintf(stderr, "Opening %s\n", out_file);
        state.out_fp = fopen(out_file, "w+");
    } else {
        state.out_fp = fdopen(STDOUT_FILENO, "w");
    }
    if (state.out_fp == NULL)
        err(1, "open(%s)", out_file);


    fprintf(state.out_fp, "# Started at: %s, Version: %s, Generated by: %s\n",
            current_time().c_str(), GIT_VERSION,
            shell_quote(argc, argv).c_str());

    if(write_stdout)
        stdout_column = &(columns.add("stdout"));
    CsvRow row(columns);
    columns.setHeader(row);
    row.write(state.out_fp);

    // Clear signal mask in children - don't let them inherit our
    // mask, which libev "randomly" modifies
    pthread_atfork (0, 0, clear_sig_mask);

    measure(measure_period_ms);

    fclose(state.out_fp);

    if (strcmp(out_file, "-") != 0)
        fprintf(stderr, "Results stored to %s\n", out_file);

    return 0;
}
