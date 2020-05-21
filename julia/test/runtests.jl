import Pkg
Pkg.activate("/home/wsh/thermac/thermobench/julia")
using Thermobench
const T = Thermobench

using Documenter
doctest(Thermobench, manual=false)
# doctest(Thermobench, manual=false)

using Gnuplot, LsqFit, DataFrames
cd("/home/wsh/thermac/devel/experiments")

## test bad fit
f = T.plot_fit("memory-bandwidth/data-fan/rnd-a53-t1-s16k.csv", :CPU_0_temp_°C,
               order=2, plotexp=true, attempts = 1,
               tau_bounds=[(30, 120), (10*60, 20*60)],
               T_bounds=(30, 50),
               k_bounds=[(-10,+10), (-5,+5)],
               show_trace = true,
               use_cmpfit = true,
               );


## Rest
csvs=String[]
for (root, dirs, files) in walkdir("memory-bandwidth/data-fan-nowork")
    for file in files
        if splitext(file)[2] == ".csv"
            push!(csvs, joinpath(root, file))
        end
    end
end
T.plot_fit(csvs, :CPU_0_temp_°C, order=2)

tasks = Dict("a53" => 1:1,#4,
             "a72" => 1:1,#2,
             "all" => 6);
csvs=["memory-bandwidth/data-$data/$ord-$cpu-t$t-s$s.csv"
      for data in ["fan", "nofan"] #["fan-nowork" "fan" "nofan"]
      for cpu in ["a53" "a72" "all"]
      for s in ["16k" "256k" "4M"]
      for t in tasks[cpu]
      for ord in ["seq" "rnd"]
      ];
T.plot_fit(csvs, :CPU_0_temp_°C, order=2)

# Thermocam calibration/correction
tasks = Dict("a53" => 1:4,
             "a72" => 1:2,
             "all" => 6);
csvs=["memory-bandwidth/data-$data/$ord-$cpu-t$t-s$s.csv"
      for data in ["nofan"] #["fan-nowork" "fan" "nofan"]
      for cpu in ["a53" "a72" "all"]
      for s in ["16k" "256k" "4M"]
      for t in tasks[cpu]
      for ord in ["seq" "rnd"]
      ];
calib = Array{Float64}(undef, 0, 2)
for csv in csvs
    global calib
    df = T.read(csv)
    x = T.thermocam_correct!(df)'
    calib = vcat(calib, x)
end
describe(DataFrame(calib), :all)

# Visualize thermo camera callibration
df = T.read("memory-bandwidth/data-nofan/rnd-a53-t1-s16k.csv")
x = T.thermocam_correct!(df)
cam_cols = [:cam_cpu, :cam_mem, :cam_board, :cam_table]
d = df[:, [:time_s, cam_cols...]] |> dropmissing
d_amb = df[:, [:time_s, :ambient_°C]] |> dropmissing
@gp("set term qt noraise", "set grid",
    d_amb.time_s, d_amb.ambient_°C, "w lp title 'ambient'",
    df.time_s, df.CPU_0_temp_°C, "w p title 'CPU0'",
    d.time_s, d.cam_cpu, "w lp title 'cam'",
    d.time_s, d.cam_mem, "w lp title 'mem'",
    d.time_s, d.cam_board, "w lp title 'board'",
    d.time_s, d.cam_table, "w lp title 'table'"
    )
x

cols = [:CPU_0_temp_°C #, :CPU_1_temp_°C, :GPU_0_temp_°C, :GPU_1_temp_°C, :DRC_temp_°C,
        ]
T.plot_fit("freq-read/data/imx8/core1234freq1104.csv", :cpu_thermal0_°C, order=2)

f=T.plot_fit("cl-mem/cl-mem-read.csv", [:CPU_0_temp_°C :CPU_1_temp_°C], order=3, plotexp=true)

T.plot_fit(["memory-bandwidth/data-fan/seq-all-t6-s$(s).csv" for s in ["16k" "256k" "4M"]], cols, order=2)

T.plot_fit(["memory-bandwidth/data-fan/seq-a53-t$t-s$(s).csv" for s in ["16k" "256k" "4M"] for t in 1:2], cols, order=2)

T.plot_fit(["memory-bandwidth/data-fan-nowork/seq-a53-t$t-s$(s).csv" for s in ["16k" "256k" "4M"] for t in 1:2], cols, order=2)

T.plot_fit(["memory-bandwidth/data-nofan/seq-a53-t$t-s$(s).csv" for s in ["16k" "256k" "4M"] for t in 2:2:4], cols, order=2)

f = T.plot_fit(["memory-bandwidth/data-nofan/seq-a53-t2-s4M.csv"],
               [:CPU_0_temp_°C :cam_cpu],
               order=2)

tau_bounds = [
    (10, 2*60),
    (2*60, 10*60),
    (10*60, 60*60)
]

df = T.read("thermocam/thermocam-gpu.csv")
T.plot_fit(filter(df) do row; 26 <= row.time_s/60 <= 48 end, :cam_cpu, order=3)

T.plot_fit("./thermocam/thermocam.csv", :cam_cpu)

fs=[
    "random-global-parallel.csv",
    "random-global-serial.csv",
    "random-shared-parallel.csv",
    "random-shared-serial.csv",
]
T.plot_fit("joel/glob-shared/" .* fs,
           @T.symarray(Cortex_A57_temp_°C),#,Denver2_temp_°C,GPU_temp_°C,PLL_temp_°C,Tboard_temp_°C,Tdiode_temp_°C,Thfan_temp_°C),
           order=2)
