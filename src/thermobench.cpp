//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (C) 2019 Czech Technical University in Prague
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
#define EV_STANDALONE 1
#include "libev/ev.h"

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
};

/* Command line options */
int measure_period_ms = 100;
char *benchmark_path[2] = { NULL, NULL };
char **benchmark_argv = NULL;
char *bench_name = NULL;
const char *output_path = ".";
char *out_file = NULL;
bool write_stdout = false;
int terminate_time = 0;

struct measure_state {
    struct timespec start_time;
    vector<sensor> sensors;
    FILE *out_fp;
    vector<char*> keys;
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
        parse_sensor_spec(&sensor, line);
        state.sensors.push_back(sensor);
    }
    if (line)
        free(line);

    fclose(fp);
}

void write_header_csv(FILE *fp, vector<sensor> &sensors, vector<char*> &keys, bool write_stdout)
{
    fprintf(fp, "time/ms");
    for (unsigned i = 0; i < sensors.size(); ++i)
        fprintf(fp, sensors[i].units ? ",%s/%s" : ",%s", sensors[i].name, sensors[i].units);
    for (unsigned i = 0; i < keys.size(); ++i) {
        fprintf(fp, ",%s", keys[i]);
    }
    if (write_stdout)
        fprintf(fp, ",stdout");
    fprintf(fp, "\n");
}

void write_column_csv(FILE *fp, double time, char *string, int column)
{
    fprintf(fp, "%lf,", time);
    for (int i = 1; i < column; ++i)
        fprintf(fp, ",");
    fprintf(fp, "%s\n", string);
}

void write_arr_csv(FILE *fp, double *arr, int size)
{
    fprintf(fp, "%g", arr[0]);
    for (int i = 1; i < size; ++i) {
        fprintf(fp, ",");
        if (!isnan(arr[i]))
            fprintf(fp, "%g", arr[i]);
    }
    fprintf(fp, "\n");
}

void rewrite_header_keys_csv(FILE *fp, char *keys[], int num_keys, int num_sensors)
{
    size_t len;
    char *line = NULL;

    fseek(fp, 0, SEEK_SET);
    if ((getline(&line, &len, fp)) == -1)
        err(1, "getline");

    char *s = strchr(line, ',');
    for (unsigned i = 0; i < state.sensors.size() + 2; ++i) {
        s = strchr(s, ',');
        s++;
    }

    *(s - 1) = '\0';
    for (int i = 0; i < num_keys; ++i) {
        strcat(line, ",");
        strncat(line, keys[i], MAX_KEY_LENGTH);
    }
    memset(line + strlen(line), ' ', len - strlen(line));
    line[len - 2] = '\n';

    fseek(fp, 0, SEEK_SET);
    fwrite(line, sizeof(char), len - 1, fp);

    if (line)
        free(line);
}

void set_process_affinity(int pid, int cpu_id)
{
    cpu_set_t my_set;
    CPU_ZERO(&my_set);
    CPU_SET(cpu_id, &my_set);
    sched_setaffinity(pid, sizeof(cpu_set_t), &my_set);
}

int get_key_idx(char *key, vector<char*> &keys)
{
    if (keys.size() == 0)
        return -1;
    for (unsigned i = 0; i < keys.size(); ++i)
        if (strcmp(key, keys[i]) == 0)
            return i;
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
    double curr_time = get_current_time();
    while (fscanf(workfp, "%[^\n]", buf) > 0) {
        char *eq = strchr(buf, '=');
        if (eq) {
            *eq = 0;

            char *key = buf;
            char *value = eq + 1;

            int id = get_key_idx(key, state.keys);

            if (id >= 0) {
                write_column_csv(state.out_fp, curr_time, value,
                                 1 + state.sensors.size() + id);
                return;
            }
            *eq = '=';
        }
        if (write_stdout)
            write_column_csv(state.out_fp, curr_time, buf,
                             1 + state.sensors.size() + state.keys.size());
    }
}

static void child_exit_cb(EV_P_ ev_child *w, int revents)
{
    ev_break(EV_A_ EVBREAK_ALL);
}

static void measure_timer_cb(EV_P_ ev_timer *w, int revents)
{
    double res_buf[MAX_RESULTS] = { 0 };

    res_buf[0] = get_current_time();
    //    printf("%g\n", get_current_time());

    for (unsigned i = 0; i < state.sensors.size(); ++i) {
        FILE *fp = fopen(state.sensors[i].path, "r");
        if (fp == NULL)
            err(1, "Error while opening sensor file: %s", state.sensors[i].path);
        int scanret = fscanf(fp, "%lf", &res_buf[i + 1]);
        if (scanret != 1) // read fail or file empty
            res_buf[i + 1] = NAN;
        fclose(fp);
    }
    write_arr_csv(state.out_fp, res_buf, state.sensors.size() + 1);
}

static void terminate_timer_cb(EV_P_ ev_timer *w, int revents)
{
    kill(state.child, SIGTERM);
}

static void sigint_cb(struct ev_loop *loop, ev_signal *w, int revents)
{
    // Ignore SIGINT. But our child will not ignore it and exits. We
    // detect that in child_exit_cb and break the event loop.
    fprintf(stderr, "Waiting for child to terminate...\n");
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

    int currpriority = getpriority(PRIO_PROCESS, getpid());
    setpriority(PRIO_PROCESS, getpid(), currpriority - 1);

    set_process_affinity(pid, 0);

    clock_gettime(CLOCK_MONOTONIC, &state.start_time);

    ev_run(loop, 0);
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
        state.keys.push_back(arg);
        break;
    case 't':
        terminate_time = atoi(arg);
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
    { "benchmark",      'b', "EXECUTABLE",  0, "Benchmark program to execute" },
    { "benchmark_path", 'b', 0,             OPTION_ALIAS | OPTION_HIDDEN },
    { "sensors_file",   's', "FILE",        0, "Definition of sensors to read" },
    { "sensor",         'S', "SPEC",        0, "Add sensor to the list of read sensors "
                                               "(SPEC is typically something like: "
                                               "/sys/devices/virtual/thermal/thermal_zone0/temp [NAME [UNIT]])" },
    { "name",           'n', "NAME",        0, "Basename of the .csv file" },
    { "bench_name",     'n', 0,             OPTION_ALIAS | OPTION_HIDDEN },
    { "output_dir",     'o', "DIR",         0, "Where to create output .csv file" },
    { "output",         'O', "FILE",        0, "The name of output CSV file" },
    { "column",         'c', "STR",         0, "Add column to CSV populated by STR=val lines from COMMAND stdout" },
    { "stdout",         'l', "STR",         0, "Log COMMAND stdout to CSV" },
    { "time",           't', "SECONDS",     0, "Terminate the COMMAND after this time" },
    { 0 }
};

const char * argp_program_bug_address = "https://github.com/CTU-IIG/thermobench/issues";
const char * argp_program_version = GIT_VERSION;

/* Our argp parser. */
static struct argp argp = {options, parse_opt, "[--] COMMAND...", ""};

int main(int argc, char **argv)
{
    argp_parse(&argp, argc, argv, 0, 0, NULL);

    if (!out_file)
        asprintf(&out_file, "%s/%s.csv", output_path, bench_name);

    state.out_fp = fopen(out_file, "w+");
    if (state.out_fp == NULL)
        err(1, "fopen(%s)", out_file);

    fprintf(state.out_fp, "# Generated by: %s\n", shell_quote(argc, argv).c_str());

    write_header_csv(state.out_fp, state.sensors, state.keys, write_stdout);

    measure(measure_period_ms);

    fclose(state.out_fp);

    fprintf(stderr, "Results stored to %s\n", out_file);

    return 0;
}
