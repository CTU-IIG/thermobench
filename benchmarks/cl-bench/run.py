#!/usr/bin/python3

import datetime
import math
import os
import pexpect
import subprocess
import sys


def powrange(low, high):
    """*inclusive* range of powers of 2"""
    low, high = map(int, map(math.log2, [low, high]))
    return list(map(lambda e: 2**e, range(low, high+1)))


def build(makeargs, target):
    """copies the project to a build VM, builds it with the yocto toolchain and pulls it back as {target}"""

    os.environ["TERM"] = "dumb"
    subprocess.run(["sshpass", "-pasdf", "ssh", "user@vm", "rm", "-rf", "cl-bench"]).check_returncode()
    subprocess.run(["sshpass", "-pasdf", "rsync", "-av", ".", "user@vm:~/cl-bench"]).check_returncode()
    vm = pexpect.spawn("sshpass", ["-pasdf", "ssh", "user@vm"])
    vm.logfile = sys.stdout.buffer
    vm.expect_exact("~$ ")
    vm.sendline("cd cl-bench")
    vm.expect_exact("~/cl-bench$ ")
    vm.sendline("make clean")
    vm.expect_exact("~/cl-bench$ ")
    vm.sendline("source yocto_env.sh")
    vm.expect_exact("~/cl-bench$ ")
    vm.sendline(f"make {makeargs} target/{target}")
    vm.expect_exact("~/cl-bench$ ")
    vm.sendeof()
    subprocess.run(["sshpass", "-pasdf", "rsync", "-av", f"user@vm:~/cl-bench/target/{target}", target]).check_returncode()


def run(benchmark, timeout, suffix):
    """runs benchmark on target board for timeout seconds and saves output to a file with suffix"""

    print("@@@@", datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))

    waittemp = 32 # deg C

    subprocess.run(["rsync", "-av", benchmark, f"ritchie:~/imx8_data/root/{benchmark}"]).check_returncode()
    subprocess.run([
        "sshpass", "-pasdf",
        "ssh", "root@imx8",
        "./thermobench",
        "--sensors=sensors.imx8",
        f"--output={benchmark}-{suffix}.csv",
        f"--time={timeout}",
        "--column=work_done",
        f"--wait={waittemp}", "--wait-timeout=300",
        "'--fan-cmd=ssh imx8fan@c2c1'", "--fan-on",
        "--",
        f"./{benchmark}",
    ]).check_returncode()
    

if True: # runall cl-mem benchmarks
    sizes = [ (local_ws, global_ws) for global_ws in powrange(32, 16384) for local_ws in powrange(32, 1024) if local_ws <= global_ws ]
    memsize = 8 * 1024**2 # MB
    blocksize = 64
    reps = 1024
    timeout = 600
    kernel = "read"

    for i, size in enumerate(sizes):
        print(f"######## BENCH {i+1}/{len(sizes)} :: {str(size)} ########")
        local_ws, global_ws = size
        build(f"GLOBAL_WS={global_ws} LOCAL_WS={local_ws} REPS={reps} MEMSIZE={memsize} BLOCKSIZE={blocksize} KERNEL={kernel}", "cl-mem")
        run("cl-mem", timeout, f"{global_ws}-{local_ws}")


if False: # runall cl-mandelbrot benchmarks
    sizes = [ (local_ws, global_ws) for global_ws in powrange(32, 16384) for local_ws in powrange(32, 1024) if local_ws <= global_ws ]
    memsize = 2048
    timeout = 600

    for i, size in enumerate(sizes):
        print(f"######## BENCH {i+1}/{len(sizes)} :: {str(size)} ########")
        local_ws, global_ws = size
        build(f"GLOBAL_WS={global_ws} LOCAL_WS={local_ws} REPS=1 MEMSIZE={memsize} BLOCKSIZE=1 KERNEL=read", "cl-mandelbrot")
        run("cl-mandelbrot", timeout, f"{global_ws}-{local_ws}")
