var documenterSearchIndex = {"docs":
[{"location":"#Thermobench.jl","page":"Thermobench.jl","title":"Thermobench.jl","text":"","category":"section"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"DocTestSetup = :(using Thermobench, DataFrames)","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"using Gnuplot, Pipe, DataFramesMeta\nGnuplot.quitall()\nmkpath(\"build/assets\")\nGnuplot.options.term = \"unknown\"\nempty!(Gnuplot.options.init)\npush!(Gnuplot.options.init, linetypes(:Set1_5, lw=1.5, ps=1.5))\npush!(Gnuplot.options.init, \"set errorbars lw 2\")\nsaveas(file; width=800, height=350) = save(term=\"pngcairo size $width,$height fontscale 0.8\", output=\"build/assets/$(file).png\")","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"Julia module for working with thermobench-produced CSV files.","category":"page"},{"location":"#Contents","page":"Thermobench.jl","title":"Contents","text":"","category":"section"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"Depth = 4","category":"page"},{"location":"#Installation","page":"Thermobench.jl","title":"Installation","text":"","category":"section"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"Thermobench.jl can be installed using the Julia package manager. From the Julia REPL, type ] to enter the Pkg REPL mode and run:","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"pkg> add https://github.com/CTU-IIG/thermobench:julia","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"Alternatively, if you want to use the same versions of packages as the author of the package, run:","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"(@v1.6) pkg> activate /path/to/thermobench/julia\n(Thermobench) pkg> instantiate","category":"page"},{"location":"#Usage","page":"Thermobench.jl","title":"Usage","text":"","category":"section"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"Thermobench package supports plotting with Gnuplot.jl package so you will most likely need both packages:","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"using Thermobench, Gnuplot","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"You can also create a shortcut T for accessing non-exported members of Thermobench package.","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"const T = Thermobench\nnothing # hide","category":"page"},{"location":"#High-level-data-processing-and-graphing","page":"Thermobench.jl","title":"High-level data processing and graphing","text":"","category":"section"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"The simplest way to using the package is the multi_fit function. In the example below, it reads the data from a CSV file and fits a thermal model to it. The result can be directly plotted by Gnuplot.jl:","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"@gp multi_fit(\"test1.csv\")\nsaveas(\"test-mf\") # hide","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"(Image: )","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"You can pass multiple CSV files to multi_fit. The result is shown as a DataFrame, which makes it easy to compare the results. You can also specify additional keywords to control the operation. Below we use subtract to subtract ambient temperature from the data to fit, use_measurements to produce results with confidence intervals and use_cmpfit to use alternative fitting solver.","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"mf = multi_fit([\"test1.csv\", \"test2.csv\"], subtract=:ambient, use_cmpfit=true, use_measurements=true)\n@gp mf\nsaveas(\"test-mf2\") # hide","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"(Image: )","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"In most cases, we are interested only in T_ parameters. These can be plot (and compared between multiple data sets) with plot_Tinf.","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"mf2 = multi_fit([\"test1.csv\", \"test2.csv\"], :CPU_1_temp, name=\"CPU1\", subtract=:ambient, use_cmpfit=true, use_measurements=true)\n@gp plot_Tinf(rename!(mf, \"CPU0\"), mf2) key=\"left\"\nsaveas(\"tinf\", width=400) # hide","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"(Image: )","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"Both T_ and benchmark performance can be plotted with plot_Tinf_and_ops:","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"@gp plot_Tinf_and_ops(mf2) key=\"left\"\nsaveas(\"tinf-ops\", width=480) # hide","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"(Image: )","category":"page"},{"location":"#Raw-thermobench-data","page":"Thermobench.jl","title":"Raw thermobench data","text":"","category":"section"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"To access raw data from thermobench CSV files, use the Thermobench.read function:","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"using DataFrames\nd = T.read(\"test1.csv\");\ndump(d, maxdepth=1)\nfirst(d.df, 6)","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"In the example above, the data is available in d.df DataFrame, where you can manipulate them as you want. You can find a lot of examples in DataFrames.jl documentation.","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"To write (possibly modified) data to a file, use Thermobench.write function.","category":"page"},{"location":"#Plotting","page":"Thermobench.jl","title":"Plotting","text":"","category":"section"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"You can plot the data by using directly the values from DataFrame d.df, but the plot(::Thermobench.Data) method makes it easier:","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"@gp    plot(d, :CPU_0_temp) key=\"left\"\n@gp :- plot(d, :ambient, with=\"lines\", title=\"Amb. temperature\")\nsaveas(\"raw-cpu\") # hide","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"(Image: )","category":"page"},{"location":"#Missing-values-and-interpolation","page":"Thermobench.jl","title":"Missing values and interpolation","text":"","category":"section"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"If you want to get rid of missing data, you can select interesting columns and pass the dataframe through dropmissing:","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"select(d.df, [:time, :CPU_0_temp]) |> dropmissing","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"which is the same as:","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"dropmissing(select(d.df, [:time, :CPU_0_temp]));","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"or","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"using Pipe: @pipe\n@pipe d.df |> select(_, [:time, :CPU_0_temp]) |> dropmissing(_);","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"Note that when you select all columns, you will likely end up with empty dataframe, because dropmissing keeps only rows with no missing values.","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"Alternatively, you can get rid of missing data by interpolating them with interpolate:","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"interpolate(d)","category":"page"},{"location":"#Other-useful-data-manipulations","page":"Thermobench.jl","title":"Other useful data manipulations","text":"","category":"section"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"To filter out some rows, you can use:","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"using DataFramesMeta\n@linq d.df |> where(10.0 .< :time .< 13.0)","category":"page"},{"location":"#Reference","page":"Thermobench.jl","title":"Reference","text":"","category":"section"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"","category":"page"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"Modules = [Thermobench]","category":"page"},{"location":"#Thermobench.Data","page":"Thermobench.jl","title":"Thermobench.Data","text":"mutable struct Data\n    df::DataFrame\n    name::String                # label for plotting\n    meta::Dict\nend\n\nData read from thermobench CSV file.\n\n\n\n\n\n","category":"type"},{"location":"#Thermobench.Data-Tuple{Thermobench.Data,AbstractDataFrame}","page":"Thermobench.jl","title":"Thermobench.Data","text":"Copy existing Data d, but replace the DataFrame with df.\n\n\n\n\n\n","category":"method"},{"location":"#Thermobench.MultiFit","page":"Thermobench.jl","title":"Thermobench.MultiFit","text":"Stores results of processing of one or more thermobench CSV files.\n\nThe main data is stored in the result field.\n\n\n\n\n\n","category":"type"},{"location":"#DataFrames.rename!-Tuple{Thermobench.MultiFit,Any}","page":"Thermobench.jl","title":"DataFrames.rename!","text":"rename!(mf::MultiFit, name)\n\nRename MultiFit data structure.\n\nThe name is often used as graph label so renaming can be used to set descriptive graph labels.\n\n\n\n\n\n","category":"method"},{"location":"#Thermobench.fit-Tuple{Array{Float64,1},Any}","page":"Thermobench.jl","title":"Thermobench.fit","text":"fit(\n    time_s::Vector{Float64},\n    data::Vector{Float64};\n    order::Int64 = 2,\n    p0 = nothing,\n    tau_bounds = [(1, 60*60)],\n    k_bounds = [(-120, 120)],\n    T_bounds = (0, 120),\n    use_cmpfit::Bool = true,\n )\n\nFit a thermal model to time series given by time_s and data. The thermal model has the form of\n\nT(t) = T_ + sum_i=1^orderk_ie^-fractτ_i\n\nwhere T_∞, kᵢ and τᵢ are the coefficients found by this function.\n\nIf use_cmpfit is true (the default), use CMPFit.jl package rather than LsqFit.jl. LsqFit doesn't work well in constrained settings.\n\nYou can limit the values of fitted parameters with *_bounds parameters. Each bound is a tuple of lower and upper limit. T_bounds limits the T∞ parameter. tau_bounds and k_bounds limit the coefficients of exponential functions ke^-tτ. If you specify less tuples than the order of the model, the last limit will be repeated.\n\nExample\n\njulia> using StatsBase: coef\n\njulia> d = Thermobench.read(\"test.csv\");\n\n\njulia> f = fit(d.df.time, d.df.CPU_0_temp);\n\n\njulia> coef(f)\n5-element Array{Float64,1}:\n  53.000281317694906\n  -8.162698631078944\n  59.36604041500533\n -13.124669051563407\n 317.6295650259018\n\njulia> printfit(f)\n\"53.0 – 8.2⋅e^{−t/59.4} – 13.1⋅e^{−t/317.6}\"\n\n\n\n\n\n","category":"method"},{"location":"#Thermobench.interpolate!-Tuple{Thermobench.Data}","page":"Thermobench.jl","title":"Thermobench.interpolate!","text":"interpolate!(d::Data)\ninterpolate!(df::AbstractDataFrame)\n\nIn-place version of interpolate.\n\n\n\n\n\n","category":"method"},{"location":"#Thermobench.interpolate-Tuple{Thermobench.Data}","page":"Thermobench.jl","title":"Thermobench.interpolate","text":"interpolate(d::Data)\ninterpolate(df::AbstractDataFrame)\n\nReplace missing values with results of linear interpolation performed against the first column (time).\n\njulia> x = DataFrame(t=[0.0, 1, 2, 3, 1000, 1001], v=[0.0, missing, missing, missing, 1000.0, missing])\n6×2 DataFrame\n Row │ t        v\n     │ Float64  Float64?\n─────┼────────────────────\n   1 │     0.0        0.0\n   2 │     1.0  missing\n   3 │     2.0  missing\n   4 │     3.0  missing\n   5 │  1000.0     1000.0\n   6 │  1001.0  missing\n\njulia> interpolate(x)\n6×2 DataFrame\n Row │ t        v\n     │ Float64  Float64?\n─────┼────────────────────\n   1 │     0.0        0.0\n   2 │     1.0        1.0\n   3 │     2.0        2.0\n   4 │     3.0        3.0\n   5 │  1000.0     1000.0\n   6 │  1001.0  missing\n\n\n\n\n\n\n","category":"method"},{"location":"#Thermobench.multi_fit","page":"Thermobench.jl","title":"Thermobench.multi_fit","text":"multi_fit(sources, columns = :CPU_0_temp;\n          name = nothing,\n          timecol = :time,\n          use_measurements = false,\n          order::Int64 = 2,\n          subtract = nothing,\n          kwargs...)::MultiFit\n\nCall fit() for all sources and report the results (coefficients etc.) in DataFrame. When use_measurements is true, report coefficients with their confidence intervals as Measurement objects.\n\nsubtract specifies the column (symbol), which is subtracted from data after interpolating its values with interpolate. This intended for subtraction of ambient temperature.\n\njulia> multi_fit(\"test.csv\", [:CPU_0_temp :CPU_1_temp])\nThermobench.MultiFit: test.csv\n    2×9 DataFrame\n Row │ name      column      rmse       ops                Tinf     k1         ⋯\n     │ String    Symbol      Float64    Measurem…          Float64  Float64    ⋯\n─────┼──────────────────────────────────────────────────────────────────────────\n   1 │ test.csv  CPU_0_temp   0.308966  3.9364e8±280000.0  53.0003  -8.1627    ⋯\n   2 │ test.csv  CPU_1_temp  14.1697    3.9364e8±280000.0  53.7449  -8.79378\n                                                               3 columns omitted\n\n\n\n\n\n","category":"function"},{"location":"#Thermobench.normalize_units!-Tuple{Thermobench.Data}","page":"Thermobench.jl","title":"Thermobench.normalize_units!","text":"normalize_units!(d::Data)\n\nNormalize units to seconds and °C.\n\n\n\n\n\n","category":"method"},{"location":"#Thermobench.ops_est","page":"Thermobench.jl","title":"Thermobench.ops_est","text":"ops_est(d::Data, col_idx = r\"work_done\")::Measurement\n\nReturn sum of operations per second estimations calculated from multiple work_done columns. This is most often used for calculating \"performance\" of all CPUs together. The result is obtained by combining sample_mean_est and ops_per_sec for all matching columns.\n\nOptional arguments\n\ndrop_inf argument is passed to ops_per_sec function,\nalpha is passed to sample_mean_est.\n\nExample\n\njulia> ops_est(Thermobench.read(\"test.csv\"))\n3.9364e8 ± 280000.0\n\n\n\n\n\n","category":"function"},{"location":"#Thermobench.ops_per_sec","page":"Thermobench.jl","title":"Thermobench.ops_per_sec","text":"ops_per_sec(d::Data, column = :work_done; drop_inf = false)::Vector{Float64}\n\nReturn a vector of operations per second calculated by combining information from time and work_done-type column identified with column. If drop_inf is true, infinity values (if any) are removed from the resulting vector.\n\njulia> ops_per_sec(Thermobench.read(\"test.csv\"), :CPU0_work_done) |> ops->ops[1:3]\n3-element Array{Float64,1}:\n 5.499226671249357e7\n 5.498016096730722e7\n 5.4862923057965025e7\n\n\n\n\n\n","category":"function"},{"location":"#Thermobench.plot","page":"Thermobench.jl","title":"Thermobench.plot","text":"Plot various Thermobench data types.\n\n\n\n\n\n","category":"function"},{"location":"#Thermobench.plot-2","page":"Thermobench.jl","title":"Thermobench.plot","text":"plot(d::Data, columns = :CPU_0_temp; kwargs...)\n\nPlot raw values from Thermobench .csv file stored in d. data.\n\nArguments\n\ncolumns: Column or array of columns to plot (columns are specified as Julia symbols)\nwith: Gnuplot's \"with\" value – chooses how the data is plot. Defaults to \"points\".\ntitle: Custom title for all plotted columns. Default title is column title of each plot column.\nenhanced: Use Gnuplot enhanced markup for title. Default is false.\n\n\n\n\n\n","category":"function"},{"location":"#Thermobench.plot-Tuple{Thermobench.MultiFit}","page":"Thermobench.jl","title":"Thermobench.plot","text":"plot(mf::MultiFit; kwargs...)::Vector{Gnuplot.PlotElement}\n\nPlot MultiFit data.\n\nArguments\n\nminutes::Bool=true: chooses between seconds and minutes on the x-axis.\npt_titles::Bool=true whether to include titles of measured data points in separate legend column.\npt_decim::Int=1 draw only every pt_decim-th measured data point. This can be used to reduce the size of vector image formats.\npt_size::Real=1 size of measured data points.\npt_saturation::Real=0.4 intensity of points (0 is white, 1 is saturate)\nstddev::Bool=true whether to include root-mean-square error of the fit in the legend (as (±xxx)).\nmodels::Bool=true whether show thermal model expressions in the key.\n\n\n\n\n\n","category":"method"},{"location":"#Thermobench.plot_Tinf-Union{Tuple{Vararg{Thermobench.MultiFit,N}}, Tuple{N}} where N","page":"Thermobench.jl","title":"Thermobench.plot_Tinf","text":"plot_Tinf(mfs::MultiFit...; kwargs...)::Vector{Gnuplot.PlotElement}\n\nPlot T_ as bargraphs. Multiple data sets can be passed as arguments to compare them. kwargs are passed to Thermobench.plot_bars.\n\n\n\n\n\n","category":"method"},{"location":"#Thermobench.plot_Tinf_and_ops-Union{Tuple{Thermobench.MultiFit}, Tuple{N}} where N","page":"Thermobench.jl","title":"Thermobench.plot_Tinf_and_ops","text":"plot_Tinf_and_ops(mf::MultiFit; kwargs...)::Vector{Gnuplot.PlotElement}\n\nPlot T_ and performance (ops per second from ops_est) as a bargraph.\n\nArguments\n\nperf_str: Title to show for the performance legend and y-axis. Default is \"Performance\".\n\n\n\n\n\n","category":"method"},{"location":"#Thermobench.plot_bars-Tuple{AbstractDataFrame}","page":"Thermobench.jl","title":"Thermobench.plot_bars","text":"plot_bars(df::AbstractDataFrame; kwargs...)::Vector{Gnuplot.PlotElement}\n\nGeneric helper function to plot bar graphs with Gnuplot.jl.\n\nArguments\n\ndf: data to plot. The first column should contains text labels, the other columns the ploted values. If the values are of Measurement type, they will be plotted with errorbars style, unless overridden with hist_style.\nbox_width=0.8: the width of the boxes. One means that the boxes touch each other.\ngap::Union{Int64,Nothing}=nothing: The gap between bar clusters. If nothing, it is set automatically depending on the number of bars in the cluster; to zero for one bar in the cluster, to 1 for multiple bars.\nhist_style=nothing: histogram style — see Gnuplot documentation.\nfill_style=\"solid 1 border -1\": fill style — see Gnuplot documentation.\nerrorbars=\"\": errorbars style — see Gnuplot documentation.\nlabel_rot=-30: label rotation angle; if > 0, align label to right.\nlabel_enhanced=false: whether to apply Gnuplot enhanced formatting to labels.\nkey_enhanced=false: whether to apply Gnuplot enhanced formatting to data keys.\ny2cols=[]: Columns (specified as symbols) which should be plot against y2 axis.\nlinetypes=1:ncol(df)-1: Line types (colors) used for different bars\n\nExample\n\njulia> using Measurements, Gnuplot\n\njulia> df = DataFrame(names=[\"very long label\", \"b\", \"c\"],\n                      temp=10:12,\n                      speed=collect(4:-1:2) .± 1)\n3×3 DataFrame\n Row │ names            temp   speed\n     │ String           Int64  Measurem…\n─────┼───────────────────────────────────\n   1 │ very long label     10    4.0±1.0\n   2 │ b                   11    3.0±1.0\n   3 │ c                   12    2.0±1.0\n\njulia> @gp Thermobench.plot_bars(df)\n\n\n\n\n\n\n","category":"method"},{"location":"#Thermobench.plot_fit","page":"Thermobench.jl","title":"Thermobench.plot_fit","text":"plot_fit(sources, columns = :CPU_0_temp;\n         timecol = :time,\n         kwargs...)\n\nCall fit for all sources and columns and produce a graph using gnuplot.\n\nnote: Note\nThis function is now superseded by multi_fit, which offers more possibilities for plotting the data.\n\nsources can be a file name (String) or a DataFrame or an array of these.\n\ntimecol is the columns with time of measurement.\n\nSetting plotexp to true causes the individual fitted exponentials to be plotted in addition to the compete fitted function.\n\nOther kwargs are passed to fit.\n\nExample\n\njulia> plot_fit(\"test.csv\", [:CPU_0_temp :CPU_1_temp], order=2);\n\n\n\n\n\n\n\n","category":"function"},{"location":"#Thermobench.printfit-Tuple{Any}","page":"Thermobench.jl","title":"Thermobench.printfit","text":"printfit(fit; minutes = false)\n\nReturn the fitted function (result of fit) as Gnuplot enhanced string. Time constants (τᵢ) are sorted from smallest to largest.\n\nExample\n\njulia> f = Thermobench.fit(collect(0.0:10:50.0), [0, 6, 6.5, 6.8, 7, 7]);\n\njulia> printfit(f)\n\"7.1 – 5.0⋅e^{−t/1.0} – 2.1⋅e^{−t/16.1}\"\n\n\n\n\n\n","category":"method"},{"location":"#Thermobench.read-Tuple{Any}","page":"Thermobench.jl","title":"Thermobench.read","text":"read(source; normalizeunits=true, stripunits=true, name=nothing, kwargs...)::Data\n\nRead thermobech CSV file source and return it as Thermobench.Data.\n\nThe source can be a file name or an IO stream. By default units are normalized with normalize_units! and stripped from column names with strip_units!. name is stored in the resulting data structure and often serves as a graph label. It name is not specified it is set (if possible) to the basename of the CSV file. kwargs are stored in the resulting structure as metadata.\n\n\n\n\n\n","category":"method"},{"location":"#Thermobench.sample_mean_est-Tuple{Any}","page":"Thermobench.jl","title":"Thermobench.sample_mean_est","text":"sample_mean_est(sample; alpha = 0.05)::Measurement\n\nCalculate sample mean estimation and its confidence interval at alpha significance level, e.g. alpha=0.05 for 95% confidence. Return Measurement value.\n\nExample\n\njulia> sample_mean_est([1,2,3])\n2.0 ± 4.3\n\n\n\n\n\n","category":"method"},{"location":"#Thermobench.strip_units!-Tuple{Thermobench.Data}","page":"Thermobench.jl","title":"Thermobench.strip_units!","text":"strip_units!(d::Data)\n\nStrip unit names from DataFrame column names.\n\nExample\n\njulia> d = Thermobench.read(\"test.csv\", stripunits=false);\n\n\njulia> names(d.df)[1:3]\n3-element Array{String,1}:\n \"time_s\"\n \"CPU_0_temp_°C\"\n \"CPU_1_temp_°C\"\n\njulia> Thermobench.strip_units!(d)\n\n\njulia> names(d.df)[1:3]\n3-element Array{String,1}:\n \"time\"\n \"CPU_0_temp\"\n \"CPU_1_temp\"\n\n\n\n\n\n","category":"method"},{"location":"#Thermobench.thermocam_correct!-Tuple{Thermobench.Data}","page":"Thermobench.jl","title":"Thermobench.thermocam_correct!","text":"thermocam_correct!(d::Data)\n\nEstimate correction for thermocamera temperatures and apply it. Return the correction coefficients.\n\nCorrection is calculated from CPU_0_temp and cam_cpu columns. This and the names of modified columns are currently hard coded.\n\n\n\n\n\n","category":"method"},{"location":"#Thermobench.write-Tuple{Any,Thermobench.MultiFit}","page":"Thermobench.jl","title":"Thermobench.write","text":"write(file, mf::MultiFit)\n\nWrite multi_fit() results to a CSV file file.\n\n\n\n\n\n","category":"method"},{"location":"#Thermobench.write-Tuple{Any}","page":"Thermobench.jl","title":"Thermobench.write","text":"write(file, d::Data)\nd |> write(file)\n\nWrite raw thermobench data d to file named file.\n\njulia> const T = Thermobench;\n\njulia> T.read(\"test.csv\") |> interpolate! |> T.write(\"interpolated.csv\");\n\n\n\n\n\n\n","category":"method"},{"location":"#Thermobench.@symarray-Tuple","page":"Thermobench.jl","title":"Thermobench.@symarray","text":"Construct array of symbols from arguments.\n\nUseful for constructing column names, e.g.,\n\njulia> @symarray Cortex_A57_temp Denver2_temp\n2-element Array{Symbol,1}:\n :Cortex_A57_temp\n :Denver2_temp\n\n\n\n\n\n","category":"macro"},{"location":"","page":"Thermobench.jl","title":"Thermobench.jl","text":"#DocTestSetup = nothing","category":"page"}]
}
