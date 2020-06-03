if isinteractive()
    import Pkg
    Pkg.activate("/home/wsh/thermac/thermobench/julia")
    using Debugger
end
using Thermobench
const T = Thermobench
using Gnuplot, LsqFit, DataFrames, Printf
if ! isinteractive()
    using Documenter
    DocMeta.setdocmeta!(Thermobench,
                        :DocTestSetup, :(using Thermobench, DataFrames; cd(joinpath(dirname(pathof(Thermobench)), "..", "test")));
                        recursive=true)
    doctest(Thermobench, manual=false, fix=true)
end
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
mf = T.multi_fit(csvs, :CPU_0_temp_°C, use_cmpfit=false, order=2)
@gp mf

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

@gp T.multi_fit("cl-mem/cl-mem-read.csv", [:CPU_0_temp_°C :CPU_1_temp_°C], order=3, use_measurements=true)
df = T.read("cl-mem/cl-mem-read.csv");
nrow(df)

csvs = "long-test/" .* vcat([["hot.$i.csv", "cold.$i.csv"] for i in 1:6]...)
csvs =  [csvs[1:2:end]; csvs[2:2:end]]
mf1 = T.multi_fit(csvs, :CPU_0_temp_°C, order=3, use_cmpfit=false, tau_bounds=[(10,60*60)], use_measurements=true)
@gp mf1 key="inside bottom" title="Absolute temperatures"
mf2 = T.multi_fit(csvs, :CPU_0_temp_°C, subtract=:ambient_°C, order=3, use_cmpfit=false, tau_bounds=[(10,60*60)], use_measurements=true)
@gp mf2 key="inside bottom" title="Subtraction of ambient temperatures"

T.plot_fit(csvs, :CPU_0_temp_°C, order=3, use_cmpfit=true)

csvs = vcat([["long-test-fan/hot.$i.csv", "long-test-fan/cold.$i.csv"] for i in 1:2]...)
csvs =  [csvs[1:2:end]; csvs[2:2:end]]
mf = T.multi_fit(csvs, :CPU_0_temp_°C, subtract=:ambient_°C, order=2, use_cmpfit=false, tau_bounds=[(10,120*60)], use_measurements=true)
@gp mf


df = T.read("long-test/hot.1.csv")
Gnuplot.options.term = "qt noraise"
times = range(10*60, last(df.time_s), length=25)
f=T.multi_fit(df, :CPU_0_temp_°C, order=3, use_cmpfit=true, tau_bounds=[(30, 60*60)])
Tinf=f.result.Tinf[1]
taus = (f.result.tau1[1], f.result.tau2[1], f.result.tau3[1])
plots = [
    (title = "3rd order, constrained τ", order = 3, tau_bounds=[(0.95tau, 1.05tau) for tau in taus])
    (title = "3rd order", order = 3, tau_bounds=[(30, 60*60)])
    (title = "2nd order", order = 2, tau_bounds=[(10,13000)])
]
@gp "set multiplot layout 5,1" :-
for p in 1:length(plots)
    mf = T.multi_fit([(filter(r->r.time_s < t, df), """$(@sprintf("%4.0f min", t/60))""") for t in times],
                     [:CPU_0_temp_°C], use_measurements=true, use_cmpfit=true,
                     tau_bounds=plots[p].tau_bounds, order=plots[p].order)
    @show mf
#     @gp :- 1 "set grid" key="bottom" xlab="time [min]" ylab="Temperature [°C]" :-
#     @gp :-    df.time_s/60 df.CPU_0_temp_°C "ps 1 lc rgb '#cc000000' t 'data'" :-
#     for i in 1:length(times)
#         @gp :- mf.time/60 T.model(mf.time, coef(mf.result.fit[i])) """w l lw 2 title '$(mf.result.name[i])' """ :-
#     end
    @gp :- 1 ylab="T∞ [°C]" "set grid" key="reverse Left left" xr=[0,60] yr=[Tinf-3,Tinf+3] :- #yr=[78,88]
    @gp :-   times/60 [m.val for m in mf.result.Tinf] [m.err for m in mf.result.Tinf] "w errorlines t '$(plots[p].title)'"
    @gp :- 2 ylab="k₁,₂ [°C]" "set grid" key="reverse Left left" xr=[0,60] yr=[-30,0] :-
    @gp :-   times/60 [m.val for m in mf.result.k1] [m.err for m in mf.result.k1] "lc $p w points t 'k₁ $(plots[p].title)'"
    @gp :-   times/60 [m.val for m in mf.result.k2] [m.err for m in mf.result.k2] "lc $p w points t 'k₂ $(plots[p].title)'"
    @gp :- 3 ylab="τ₁ [min]" "set grid" key="reverse Left left" xr=[0,60] yr=[0,2] :-
    @gp :-   times/60 [m.val/60 for m in mf.result.tau1] [m.err/60 for m in mf.result.tau1] "lc $p w errorlines t 'τ₁ $(plots[p].title)'"
    @gp :- 4 ylab="τ₂ [min]" "set grid" key="reverse Left left" xr=[0,60] yr=[0,10] :-
    @gp :-   times/60 [m.val/60 for m in mf.result.tau2] [m.err/60 for m in mf.result.tau2] "lc $p w errorlines t 'τ₂ $(plots[p].title)'"
    if plots[p].order == 3
        @gp :- 5 xlab="time [min]" ylab="τ₃ [min]" "set grid" key="reverse Left left" xr=[0,60] yr=[5,30] :-
        @gp :-   times/60 [m.val/60 for m in mf.result.tau3] [m.err/60 for m in mf.result.tau3] "lc $p w errorlines t 'τ₃ $(plots[p].title)'"
    end
end

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


@gp T.multi_fit("joel-xavier/" .* ["do_some_light_GPU_30_times.sh.fan.csv"
                              "do_some_stress_20min.sh.fan.csv"
                              "do_some_light_GPU_30_times.sh.nofan.csv"
                              "do_some_stress_20min.sh.nofan.csv"],
           :GPU_therm_°C, #@T.symarray CPU-therm GPU-therm AUX-therm AO-therm PMIC-Die Tboard_tegra Tdiode_tegra thermal-fan-est,
           order=3, use_cmpfit=true, p0=:random)
@gp :- key="invert"

dfs = [(T.read("joel-xavier-fan-speed/do_some_stress_30min.sh.fan$i.csv") |> df->df[!, r"therm"] ./= 1e3,
        "fan$i")
       for i in 0:32:256];
tau_bounds=[(3,60), (3*60,60*60)]
mf = T.multi_fit(dfs, :GPU_therm, use_cmpfit=true, tau_bounds=tau_bounds)
@gp mf "set title 'Xavier with different fan speeds'"
