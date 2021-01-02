if isinteractive()
    import Pkg
    Pkg.activate("/home/wsh/thermac/thermobench/julia")
    using Debugger
end
using Thermobench
const T = Thermobench
using Gnuplot, LsqFit, DataFrames, Printf, Measurements
using Statistics, Test, DataFramesMeta, Dates
Gnuplot.options.term = "qt noraise"
if ! isinteractive()
    using Documenter
    DocMeta.setdocmeta!(Thermobench,
                        :DocTestSetup, :(using Thermobench, DataFrames; cd(joinpath(dirname(pathof(Thermobench)), "..", "test")));
                        recursive=true)
    doctest(Thermobench, manual=false, fix=true)
end
macro gpok(args...) :( (@gp $(esc.(args)...); true) ) end
cd("/home/wsh/thermac/devel/experiments")

@testset "plot_bars" begin
    # Plain numbers
    df = DataFrame(names=["a", "b", "c"], temp=10:12, speed=4:-1:2)
    @test @gpok T.plot_bars(df)
    # Measurement values
    df = DataFrame(names=["very long label", "b", "c"], temp=10:12, speed=collect(4:-1:2) .± 1)
    @test @gpok T.plot_bars(df)
    @test @gpok T.plot_bars(df, errorbars="lw 5 lc rgb '#ff0000'")
    @test @gpok T.plot_bars(df, fill_style="pattern 1")
    @test @gpok T.plot_bars(df, gap=2)
    @test @gpok T.plot_bars(df, box_width=1)
    @test @gpok T.plot_bars(df, label_rot=0)
    @test @gpok T.plot_bars(df, label_rot=90) # visually check that labels are top-aligned
    @test @gpok T.plot_bars(df, y2cols=[:speed])
    @test @gpok T.plot_bars(df, y2cols=[:speed, :temp])
    @test @gpok T.plot_bars(df, hist_style = "rowstacked")
    @test_logs (:warn, r"columnstacked") @gp T.plot_bars(df, hist_style = "columnstacked")
    @test_throws AssertionError @gp T.plot_bars(df, y2cols=[:bad])
    # Symbols as lables
    @test @gpok T.plot_bars(DataFrame(l=[:a, :b], v=[1, 2]))
end

@testset "ops_per_sec" begin
    ops = ops_per_sec(T.read("test.csv"), :CPU0_work_done)
    @test mean(ops) ≈ 5.5e7 rtol=0.01
    @test std(ops) ≈ 6.1e4 rtol=0.01
    @test T.sample_mean_est(ops).val ≈ 5.5e7 rtol=0.01
    @test T.sample_mean_est(ops).err ≈ 1.24e5 rtol=0.01
end

@testset "plot raw data" begin
    d = T.read("test.csv")
    @test @gpok T.plot(d)
    @test @gpok T.plot(d, :CPU_1_temp)
    @test @gpok T.plot(d, [ :CPU_0_temp, :CPU_1_temp ])
    @test @gpok T.plot(d, title="Custom title")
    @test @gpok T.plot(d, title="x^2+y^2", enhanced=true)
end

@testset "raw data manipulation" begin
    d = T.read("test.csv")
    T.write("written.csv", d)
    d |> T.write("written-via-pipe.csv")
    T.read("test.csv") |> interpolate! |> T.write("written-via-pipe2.csv")

    @test begin
        d = DataFrame(time=[0,1,2], val=[1, missing, 2])
        interpolate!(d)
        d.val
    end == [1, 1.5, 2]
end


## test bad fit
f = T.plot_fit("memory-bandwidth/data-fan/rnd-a53-t1-s16k.csv", :CPU_0_temp,
               order=2, plotexp=true, attempts = 1,
               tau_bounds=[(30, 120), (10*60, 20*60)],
               T_bounds=(30, 50),
               k_bounds=[(-10,+10), (-5,+5)],
               show_trace = true,
               use_cmpfit = true,
               )
T.multi_fit("memory-bandwidth/data-fan/rnd-a53-t1-s16k.csv")

## Rest
csvs=String[]
for (root, dirs, files) in walkdir("memory-bandwidth/data-fan-nowork")
    for file in files
        if splitext(file)[2] == ".csv"
            push!(csvs, joinpath(root, file))
        end
    end
end
f = T.plot_fit(csvs, :CPU_0_temp, order=2)

mf1 = T.multi_fit(csvs, use_cmpfit=false, use_measurements=true)
mf2 = T.multi_fit(csvs, use_cmpfit=true)

mf2 |> T.write("mf.csv")

using Measurements
using Measurements: value, uncertainty
@gp value.(mf1.result.Tinf) uncertainty.(mf1.result.Tinf)

tasks = Dict("a53" => 1,#4
             "a72" => 1,#2,
             "all" => 6);
csvs=["memory-bandwidth/data-$data/$ord-$cpu-t$t-s$s.csv"
      for data in ["fan"] #["fan-nowork" "fan" "nofan"]
      for cpu in ["a53" "a72" "all"]
      for s in ["16k" "256k" "4M"]
      for t in tasks[cpu]
      for ord in ["seq" "rnd"]
      ];
mf = T.multi_fit(csvs, use_cmpfit=true, use_measurements=true, subtract=:ambient, order=2, tau_bounds=[(10,60),(60,30*60)])
@gp T.plot(mf, pt_size=0.2)

@gp palette(:Dark2_7) T.plot_Tinf(rename!(filter(r->occursin("seq", r.name), mf), "seq"),
                                  rename!(filter(r->occursin("rnd", r.name), mf), "rnd")) linetypes(:Set1_8)

csvs=["memory-bandwidth/data-$data/$ord-$cpu-t$t-s$s.csv"
      for data in ["fan"] #["fan-nowork" "fan" "nofan"]
      for cpu in ["a53" "a72" "all"]
      for s in ["16k" "256k" "4M"]
      for t in Dict("a53" => 1:4, "a72" => 1:2, "all" => 6)[cpu]
      for ord in ["seq" "rnd"]
      ];
mf = T.multi_fit(csvs, use_cmpfit=true, use_measurements=true, subtract=:ambient, order=2)
sort!(mf.result, :ops);
@gp T.plot_Tinf_and_ops(mf, label_rot=90) linetypes(:Set1_8)
@gp T.plot_bars(@with(mf.result, DataFrame(name=:name, perf=:ops ./ :Tinf)), label_rot=90) ylab="[op/s/°C]"

mf = T.multi_fit("memory-bandwidth/data-nofan/rnd-a53-t1-s16k.csv",
                 @symarray CPU_0_temp CPU_1_temp GPU_0_temp GPU_1_temp)
@gp mf
@gp T.plot_bars(mf.result[:, [:column, :Tinf]], label_rot=0) yr=[40,NaN]
@gp T.plot_Tinf(filter(r->r.column == :CPU_0_temp, mf),
                filter(r->r.column == :CPU_1_temp, mf),
                filter(r->r.column == :GPU_0_temp, mf),
                filter(r->r.column == :GPU_1_temp, mf)) yr=[40,NaN]

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
d = T.read("memory-bandwidth/data-nofan/rnd-a53-t1-s16k.csv")
x = T.thermocam_correct!(d)
cam_cols = [:cam_cpu, :cam_mem, :cam_board, :cam_table]
d_cpu = d.df[:, [:time, :CPU_0_temp]] |> dropmissing
dd = d.df[:, [:time, cam_cols...]] |> dropmissing
d_amb = d.df[:, [:time, :ambient]] |> dropmissing

@gp("set term qt noraise", "set grid",
    d_amb.time, d_amb.ambient, "w lp title 'ambient'",
    d_cpu.time, d_cpu.CPU_0_temp, "w p title 'CPU0'",
    dd.time, dd.cam_cpu, "w lp title 'cam'",
    dd.time, dd.cam_mem, "w lp title 'mem'",
    dd.time, dd.cam_board, "w lp title 'board'",
    dd.time, dd.cam_table, "w lp title 'table'"
    )
x

T.plot_fit("freq-read/data/imx8/core1234freq1104.csv", :cpu_thermal0, order=2)

f=T.plot_fit("cl-mem/cl-mem-read.csv", [:CPU_0_temp :CPU_1_temp], order=3, plotexp=true)

@gp T.multi_fit("cl-mem/cl-mem-read.csv", [:CPU_0_temp :CPU_1_temp], order=3, use_measurements=true)
d = T.read("cl-mem/cl-mem-read.csv");
nrow(d.df)

csvs = "long-test/" .* vcat([["hot.$i.csv", "cold.$i.csv"] for i in 1:6]...)
csvs =  [csvs[1:2:end]; csvs[2:2:end]]
mf1 = T.multi_fit(csvs, :CPU_0_temp, order=3, use_cmpfit=true, tau_bounds=[(10,60*60)], use_measurements=true)
@gp mf1 key="inside bottom" title="Absolute temperatures"
mf2 = T.multi_fit(csvs, :CPU_0_temp, subtract=:ambient, order=3, use_cmpfit=true, tau_bounds=[(10,60*60)], use_measurements=true)
@gp mf2 key="inside bottom" title="Subtraction of ambient temperatures"

T.plot_fit(csvs, :CPU_0_temp, order=3, use_cmpfit=true)

csvs = vcat([["long-test-fan/hot.$i.csv", "long-test-fan/cold.$i.csv"] for i in 1:2]...)
csvs =  [csvs[1:2:end]; csvs[2:2:end]]
mf = T.multi_fit(csvs, :CPU_0_temp, subtract=:ambient, order=2, use_cmpfit=false, tau_bounds=[(10,120*60)], use_measurements=true)
@gp mf


d = T.read("long-test/hot.1.csv")
Gnuplot.options.term = "qt noraise"
times = range(10*60, last(d.df.time), length=25)
f=T.multi_fit(d, :CPU_0_temp, order=3, use_cmpfit=true, tau_bounds=[(30, 60*60)])
Tinf=f.result.Tinf[1]
taus = (f.result.tau1[1], f.result.tau2[1], f.result.tau3[1])
plots = [
    (title = "3rd order, constrained τ", order = 3, tau_bounds=[(0.95tau, 1.05tau) for tau in taus])
    (title = "3rd order", order = 3, tau_bounds=[(30, 60*60)])
    (title = "2nd order", order = 2, tau_bounds=[(10,13000)])
]
@gp "set multiplot layout 5,1" :-
for p in 1:length(plots)
    mf = T.multi_fit([rename!(filter(r->r.time < t, d), """$(@sprintf("%4.0f min", t/60))""") for t in times],
                     [:CPU_0_temp], use_measurements=true, use_cmpfit=true,
                     tau_bounds=plots[p].tau_bounds, order=plots[p].order)
    @show mf
#     @gp :- 1 "set grid" key="bottom" xlab="time [min]" ylab="Temperature [°C]" :-
#     @gp :-    df.time/60 df.CPU_0_temp "ps 1 lc rgb '#cc000000' t 'data'" :-
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

T.plot_fit(["memory-bandwidth/data-fan/seq-all-t6-s$(s).csv" for s in ["16k" "256k" "4M"]], :CPU_0_temp, order=2)

T.plot_fit(["memory-bandwidth/data-fan/seq-a53-t$t-s$(s).csv" for s in ["16k" "256k" "4M"] for t in 1:2], :CPU_0_temp, order=2)

T.plot_fit(["memory-bandwidth/data-fan-nowork/seq-a53-t$t-s$(s).csv" for s in ["16k" "256k" "4M"] for t in 1:2], :CPU_0_temp, order=2)

T.plot_fit(["memory-bandwidth/data-nofan/seq-a53-t$t-s$(s).csv" for s in ["16k" "256k" "4M"] for t in 2:2:4], :CPU_0_temp, order=2)

f = T.plot_fit(["memory-bandwidth/data-nofan/seq-a53-t2-s4M.csv"],
               [:CPU_0_temp :cam_cpu],
               order=2)

tau_bounds = [
    (10, 2*60),
    (2*60, 10*60),
    (10*60, 60*60)
]

df = T.read("thermocam/thermocam-gpu.csv")
T.plot_fit(filter(df) do row; 26 <= row.time/60 <= 48 end, :cam_cpu, order=3)

T.plot_fit("./thermocam/thermocam.csv", :cam_cpu)

fs=[
    "random-global-parallel.csv",
    "random-global-serial.csv",
    "random-shared-parallel.csv",
    "random-shared-serial.csv",
]
T.plot_fit("joel/glob-shared/" .* fs,
           @T.symarray(Cortex_A57_temp),#,Denver2_temp,GPU_temp,PLL_temp,Tboard_temp,Tdiode_temp,Thfan_temp),
           order=2)


@gp T.multi_fit("joel-xavier/" .* ["do_some_light_GPU_30_times.sh.fan.csv"
                              "do_some_stress_20min.sh.fan.csv"
                              "do_some_light_GPU_30_times.sh.nofan.csv"
                              "do_some_stress_20min.sh.nofan.csv"],
           :GPU_therm, #@T.symarray CPU-therm GPU-therm AUX-therm AO-therm PMIC-Die Tboard_tegra Tdiode_tegra thermal-fan-est,
           order=3, use_cmpfit=true, p0=:random)
@gp :- key="invert"

dfs = [T.read("joel-xavier-fan-speed/do_some_stress_30min.sh.fan$i.csv", name="fan$i") for i in 0:32:256];
foreach(dfs) do d d.df[!, r"therm"] ./= 1e3 end
tau_bounds=[(3,60), (3*60,60*60)]
mf = T.multi_fit(dfs, :GPU_therm, use_cmpfit=true, use_measurements=true, tau_bounds=tau_bounds)
@gp mf "set title 'Xavier with different fan speeds'"
@gp T.plot_Tinf(mf) "set title 'Xavier with different fan speeds'"

dfs = [T.read("joel-xavier-fan-speed2/do_some_stress_40min.sh.fan$i.csv", name="fan$i") for i in 0:32:256];
foreach(dfs) do d d.df[!, r"therm"] ./= 1e3 end
filter!(dfs[1].df) do r r.time < 21*60 end
tau_bounds=[(3,60), (3*60,60*60)]
mf = T.multi_fit(dfs, :GPU_therm, use_cmpfit=true, use_measurements=true, tau_bounds=tau_bounds)
@gp T.plot(mf, pt_size=0.5) "set title 'Xavier with different fan speeds'"

@gp "set y2tics" "set y2label 'Time constant [min]'" linetypes(:Paired_11)
@gp :- T.plot_Tinf(rename!(mf, "T∞ [°C]")) title="Xavier with different fan speeds" yr=[25,NaN]
@gp :- Measurements.value.(mf.result.tau2/60) Measurements.uncertainty.(mf.result.tau2/60) "u 0:1:2 w errorlines lw 2 axes x1y2 title 'τ₂ [min]'"


dir = "fanda/parallel1-2runs-all/"
mf = [];
for it in 1:2
    push!(mf, T.multi_fit([T.read(dir * "cl-bench-$wi-$wg-$it.csv", name="it:$wi wg:$wg")
                           for wi in 256 .<< (0:3)
                           for wg in filter(n -> 64 ≤ wi/n ≤ 1024, 1 .<< (0:8))],
                          [:GPU_1_temp], order=2, use_measurements=true, use_cmpfit=true, subtract=:ambient,
                          tau_bounds=[(1,20),(30,10*60)],
                          name="iteration $it"))
    #@show std(Measurements.value.(mf[it].result.Tinf));
end
@gp T.plot(mf[1], pt_size=0.2)

@gp T.plot(filter(r->occursin("512", r.name), mf[1]), pt_size=0.2)
@gp T.plot_Tinf(mf...) linetypes(:Paired_11)

@gp xlab="speed [op./sec]" ylab="Tinf" "set grid"
map(mf) do mf
    perf = mean.(ops_per_sec.(mf.result.data))
    @show perf
    @gp :- perf mf.result.Tinf "lw 3 ps 3 title '$(mf.name)'"
end

perf = mean.(ops_per_sec.(mf[1].result.data))

m = mf[2]
@gp T.plot_bars(DataFrame(name=m.result.name,
                          Tinf=name=m.result.Tinf,
                          speed = sample_mean_est.(ops_per_sec.(m.result.data))
                          ),
                y2cols=[:speed])
@gp :- "set y2lab 'Speed [ops/s]'" #"set y2r [0:2]"
@gp :- linetypes(:Set1_8) ylab="Tinf [°C]" #yr=[0,NaN]

@gp T.plot_Tinf_and_ops(m) linetypes(:Set1_8)


csvs = ["hot-repeat/hot.$i.csv" for i in 1:8]
myfit(; kwargs...) = T.multi_fit(csvs, :CPU_0_temp, order=2, use_cmpfit=true, tau_bounds=[(10,60),(60,10*60)], use_measurements=true; kwargs...)
mf1 = myfit()
mf2 = myfit(subtract=:ambient)
describe(DataFrame(mf1=mf1.result.rmse, mf2=mf2.result.rmse))
std.([mf1.result.Tinf, mf2.result.Tinf])

@gp plot_Tinf(mf2) yr=[0,NaN] linetypes(:Set1_8)
@gp mf2 key="inside bottom" title="Absolute temperatures"


csvs = [T.read("fan-repeat/repeat.$i.csv") for i in 1:28];
myfit(; kwargs...) = T.multi_fit(csvs, :CPU_0_temp, order=2, use_cmpfit=true, tau_bounds=[(10,10*60)], use_measurements=true; kwargs...)
mfabs = myfit()
mfrel = myfit(subtract=:ambient)
describe(DataFrame(abs=mfabs.result.rmse, rel=mfrel.result.rmse))
std.([mf1.result.Tinf, mf2.result.Tinf])

@gp T.plot(mfrel)
@gp plot_Tinf(mfrel, label_rot=90)

@gp plot_Tinf(mf2) yr=[NaN,NaN] linetypes(:Set1_8)
@gp mf1 key="inside bottom" title="Absolute temperatures"

start = csvs[1].meta["datetime"]
@gp "set grid" key="bottom"
foreach(csvs) do csv
    s = Second(csv.meta["datetime"]-start)
    df = @select(csv.df, :time, :ambient) |> dropmissing
    @gp :- (df.time .+ s.value) df.ambient "w l title '$(csv.name)'"
end

@gp yr=[24,34] "set multiplot layout 1,4" "set grid"
for i in 1:4
    @gp :- i T.plot(csvs[i], :CPU_0_temp)
    @gp :- i T.plot(csvs[i], :ambient, with="lines")
end

@select(csvs[1].df, :time, :CPU0_work_done) |> dropmissing

names(csvs[1].df)
mean.(ops_per_sec.(csvs[1], [:CPU0_work_done, :CPU1_work_done]))

d = T.read("fanda/parallel1-2runs-all/cl-bench-1024-2-1.csv")
speed = ops_per_sec(d)
describe(speed)
@gp hist(speed, nbins=30)


d = [T.read("long-test/hot.$i.csv") for i in 1:5];
ops = vcat(ops_per_sec.(d, :CPU0_work_done)...)
@gp hist(ops, nbins=10)
