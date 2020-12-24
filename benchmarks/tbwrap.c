#include "tbwrap.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <argz.h>
#include <argp.h>
#include <err.h>
#include <time.h>
#include <stdbool.h>

/*

Wrapper for better integration of benchmarks with Thermobench.

Its function is controlled by the TB_OPTS environment variable, which
contains space-separated options. Set TB_OPTS to --help for more
information. For example:

    TB_OPTS="--help" ./tacle_bench_kernel_binarysearch

*/

struct arguments {
    uint64_t count;
    char *work_done_str;
    uint64_t work_done_every;
    bool time;
};

/* Program documentation. */
static char doc[] =
    "TB_OPTS environment variable can contain the following space-separated options. "
    "Note that there is no support for quoting, i.e. including space in the option argument.";

/* A description of the arguments we accept. */
static char args_doc[] = "";

/* The options we understand. */
static struct argp_option options[] = {
    {"count",           'c', "NUM",   0, "Execute the benchmark NUM times. Zero means infinity. Defaults to 1." },
    {"work_done_str",   'w', "STR",   0, "\"work_done\" prefix string. Empty (default) means don't print the work_done message" },
    {"work_done_every", 'e', "NUM",   0, "Print \"work_done\" message every NUM iterations. Defaults to 1." },
    {"time",            't', 0,       0, "Measure and print execution time of the benchmark." },

    { 0 }
};

/* Parse a single option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
    /* Get the ‘input’ argument from ‘argp_parse’, which we
       know is a pointer to our arguments structure. */
    struct arguments *arguments = state->input;

    switch (key)
    {
    case 'c': case 's':
        arguments->count = atoll(arg);
        break;
    case 'w':
        arguments->work_done_str = strdup(arg);
        break;
    case 'e':
        arguments->work_done_every = atoll(arg);
        break;
    case 't':
        arguments->time = true;
        break;
    case ARGP_KEY_ARG:
        return ARGP_ERR_UNKNOWN;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };

static struct arguments arguments;

static void parse_tb_opts()
{
    char *argz, **argv;
    int argc;
    size_t argz_len;
    error_t error;
    char *opts = getenv("TB_OPTS");

    /* Default values other than zeros. */
    arguments.count = 1;
    arguments.work_done_every = 1;

    if (!opts)
        opts = "";

    error = argz_create_sep(opts, ' ', &argz, &argz_len);
    if (error)
        errx(1, "argz_create_sep: %s", strerror(error));

    /* prepend argv[0] */
    argz_insert(&argz, &argz_len, argz, "");
    if (error)
        errx(1, "argz_insert: %s", strerror(error));

    argc = argz_count(argz, argz_len);
    argv = malloc((argc + 1) * sizeof (char *));
    if (!argv)
        err(1, "parse_tb_opts");

    argz_extract(argz, argz_len, argv);

    /* Parse our arguments; every option seen by ‘parse_opt’ will be
       reflected in ‘arguments’. */
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    free(argv);
    free(argz);
}

static void print_work_done(uint64_t work_done)
{
    static uint64_t wd_cntr = 1;

    if (arguments.work_done_str && --wd_cntr == 0) {
        printf("%s=%lu\n", arguments.work_done_str, work_done);
        fflush(stdout);
        wd_cntr = arguments.work_done_every;
    }
}

struct tictac {
    struct timespec tic, tac;
};

static void tic(struct tictac *t)
{
    clock_gettime(CLOCK_MONOTONIC, &t->tic);
}

static void tac(struct tictac *t)
{
    clock_gettime(CLOCK_MONOTONIC, &t->tac);

    uint64_t ns =
        + t->tac.tv_sec * 1000000000 + t->tac.tv_nsec
        - t->tic.tv_sec * 1000000000 - t->tic.tv_nsec;

    if (arguments.time)
        printf("time=%g s\n", (double)ns/1000000000.0);
}

void thermobench_wrap(void (*f)())
{
    struct tictac tictac;

    parse_tb_opts();

    tic(&tictac);

    uint64_t i;
    for (i = 0; i < arguments.count || arguments.count == 0; i++) {
        print_work_done(i);

        /* Call the benchmarked function */
        f();
    }

    tac(&tictac);
    print_work_done(i);
}
