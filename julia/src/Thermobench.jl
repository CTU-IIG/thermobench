module Thermobench

import CSV
import DataFrames: DataFrame, AbstractDataFrame, rename!, dropmissing, Not, select, nrow, ncol
using Printf, Gnuplot, Colors, Random, Dates
import LsqFit: margin_error, mse
import LsqFit
import CMPFit
import StatsBase: coef, dof, nobs, rss, stderror, weights, residuals
import Statistics: quantile, mean, std
using Measurements
using Debugger
import Distributions: TDist

export
    @symarray,
    fit,
    interpolate!,
    interpolate,
    multi_fit,
    ops_est,
    ops_per_sec,
    plot,
    plot_Tinf,
    plot_Tinf_and_ops,
    plot_fit,
    printfit,
    rename!,
    sample_mean_est

Base.endswith(sym::Symbol, str::AbstractString) = endswith(String(sym), str)

"""
```
mutable struct Data
    df::DataFrame
    name::String                # label for plotting
    meta::Dict
end
```

Data read from thermobench CSV file.
"""
mutable struct Data
    df::DataFrame
    name::String                # label for plotting
    meta::Dict
end

# Allow broadcasting Data parameters
Base.length(d::Data) = 1
Base.iterate(d::Data) = (d,1)
Base.iterate(d::Data, state) = nothing

"Copy existing Data `d`, but replace the DataFrame with `df`."
Data(d::Data, df::AbstractDataFrame) = Data(df, d.name, deepcopy(d.meta))

Base.convert(::Type{Data}, t::Tuple{AbstractDataFrame, AbstractString}) = Data(t[1], t[2], nothing, nothing, nothing, nothing)
Base.convert(::Type{Data}, df::AbstractDataFrame) = convert(Data, (df, ""))

Base.convert(::Type{DataFrame}, d::Data) = deepcopy(d.df)

function rename!(d::Data, name)
    d.name = name
    d
end

Base.show(io::IO, d::Data) = begin
    println(io, "Thermobench.Data: Name: ", d.name)
    print(io, "    ", d.df)
end

Base.filter(f, d::Data) = Data(d, filter(f, d.df))
Base.filter!(f, d::Data) = filter(f, d.df)

"Plot various Thermobench data types."
function plot end

"""
    plot(d::Data, columns = :CPU_0_temp; kwargs...)

Plot raw values from Thermobench .csv file stored in `d`.
data.

# Arguments

- `columns`: Column or array of columns to plot (columns are specified as [Julia symbols](https://docs.julialang.org/en/v1/manual/metaprogramming/#Symbols))
- `with`: Gnuplot's "with" value – chooses how the data is plot. Defaults to `"points"`.
- `title`: Custom title for all plotted columns. Default title is column title of each plot column.
- `enhanced`: Use Gnuplot enhanced markup for `title`. Default is `false`.

"""
function plot(d::Data, columns = :CPU_0_temp;
              with="points",
              title=nothing,
              enhanced=false,
              )::Vector{Gnuplot.PlotElement}
    time_div = 1
    vcat(
        Gnuplot.PlotElement(cmds="set grid"),
        map(ensurearray(columns)) do column
            df = select(d.df, [:time, column]) |> dropmissing
            Gnuplot.PlotElement(
                data=Gnuplot.DatasetText(df[!, 1]/time_div, df[!, 2]),
                plot="w $with title '$(isnothing(title) ? string(column) : title)' $(enhanced ? "" : "no")enhanced"
            )
        end
    )
end

"""
    ops_per_sec(d::Data, column = :work_done)::Vector{Float64}

Return a vector of operations per second calculated by combining
information from *time* and *work_done*-type column identified with
`column`.

```jldoctest
julia> ops_per_sec(Thermobench.read("test.csv"), :CPU0_work_done) |> ops->ops[1:3]
3-element Array{Float64,1}:
 5.499226671249357e7
 5.498016096730722e7
 5.4862923057965025e7
```
"""
function ops_per_sec(d::Data, column = :work_done)::Vector{Float64}
    wd = select(d.df, :time, column => :work_done) |> dropmissing
    speed = diff(wd.work_done) ./ diff(wd.time)
end

"""
    sample_mean_est(sample; alpha = 0.05)::Measurement

Calculate sample mean estimation and its confidence interval at
`alpha` significance level, e.g. alpha=0.05 for 95% confidence.
Return `Measurement` value.

# Example
```jldoctest
julia> sample_mean_est([1,2,3])
2.0 ± 4.3
```
"""
function sample_mean_est(sample; alpha = 0.05)::Measurement
    length(sample) == 1 && return sample[1] ± Inf
    critical_value = quantile(TDist(length(sample)-1), 1 - alpha/2)
    mean(sample) ± (std(sample) * critical_value)
end

"""
    ops_est(d::Data, col_idx = r"work_done")::Measurement

Return sum of operations per second estimations calculated from
multiple *work_done* columns. This is most often used for calculating
"performance" of all CPUs together. The result is obtained by
combining [`sample_mean_est`](@ref) and [`ops_per_sec`](@ref) for all
matching columns.

# Example
```jldoctest
julia> ops_est(Thermobench.read("test.csv"))
3.9364e8 ± 280000.0
```
"""
function ops_est(d::Data, col_idx = r"work_done")::Measurement
    wdcols = propertynames(d.df[!, col_idx])
    sum([sample_mean_est(ops)
         for ops in ops_per_sec.(d, wdcols) if length(ops) > 0])
end

"
    normalize_units!(d::Data)

Normalize units to seconds and °C."
function normalize_units!(d::Data)
    for col in propertynames(d.df)
        if col == :time_ms
            d.df[!, col] ./= 1000
            rename!(d.df, col => "time_s")
        elseif endswith(col, "_m°C")
            d.df[!, col] ./= 1000
            rename!(d.df, col => replace(String(col), r"_m°C$" => "_°C"))
        elseif endswith(col, "_mC") # Older thermobench version
            d.df[!, col] ./= 1000
            rename!(d.df, col => replace(String(col), r"_mC$" => "_°C"))
        end
    end
end

"""
    strip_units!(d::Data)

Strip unit names from DataFrame column names.

# Example
```jldoctest
julia> d = Thermobench.read("test.csv", stripunits=false);


julia> names(d.df)[1:3]
3-element Array{String,1}:
 "time_s"
 "CPU_0_temp_°C"
 "CPU_1_temp_°C"

julia> Thermobench.strip_units!(d)


julia> names(d.df)[1:3]
3-element Array{String,1}:
 "time"
 "CPU_0_temp"
 "CPU_1_temp"
```
"""
function strip_units!(d::Data)
    for col in propertynames(d.df)
        for unit in "_" .* [ "ms", "s", "m°C", "°C", "Hz", "%" ]
            endswith(col, unit) && rename!(d.df, col => (String(col)[1:end-lastindex(unit)]))
        end
    end
end

"""
    read(source; normalizeunits=true, stripunits=true, name=nothing, kwargs...)::Data

Read thermobech CSV file `source` and return it as [`Thermobench.Data`](@ref).

The `source` can be a file name or an IO stream. By default units are
normalized with [`normalize_units!`](@ref) and stripped from column
names with [`strip_units!`](@ref). `name` is stored in the resulting
data structure and often serves as a graph label. It `name` is not
specified it is set (if possible) to the basename of the CSV file.
`kwargs` are stored in the resulting structure as metadata.

"""
function read(source; normalizeunits=true, stripunits=true, name=nothing, kwargs...)::Data
    comment = readline(source)
    df = DataFrame(CSV.File(source; comment="#", normalizenames=true,
                            silencewarnings=true,
                            typemap=Dict(Int64 => Float64), # Needed for interpolation
                            ))

    function match_or_nothing(r::Regex, s::AbstractString)
        m = match(r, s)
        (m === nothing) ? nothing : m.captures[1]
    end

    dt  = match_or_nothing(r"Started at: (....-..-.. ..:..:..)", comment)
    ver = match_or_nothing(r"Version: ([^,]*)", comment)
    cmd = match_or_nothing(r"Generated by: (.*)", comment)

    meta = Dict()
    if typeof(source) <: AbstractString
        meta["filename"] = source
    end
    if ! isnothing(dt)
        meta["datetime"] =  DateTime(dt, dateformat"Y-m-d H:M:S")
    end
    if ! isnothing(ver)
        meta["version"] = ver
    end
    if ! isnothing(cmd)
        meta["command"] = cmd
    end

    # Name is always present
    nm = if name != nothing
        name
    elseif haskey(meta, "filename")
        basename(meta["filename"])
    else
        ""
    end

    d = Data(df, nm, meta)
    normalizeunits && normalize_units!(d)
    stripunits     && strip_units!(d)
    return d
end

"""
    write(file, d::Data)
    d |> write(file)

Write raw thermobench data `d` to file named `file`.

```jldoctest
julia> const T = Thermobench;

julia> T.read("test.csv") |> interpolate! |> T.write("interpolated.csv");
```
"""
write(file; kwargs...) = d->write(file, d; kwargs...)
function write(file, d::Data)
    open(file; write=true) do io
        tags = Dict("datetime" => "Started at",
                    "command" => "Generated by",
                    "version" => "Version")
        meta = d.meta
        meta["Thermobench.jl"] = true
        initial_comment = join([ "$(get(tags, key, key)): $val" for (key, val) in meta], ", ")
        println(io, "# $initial_comment")
        CSV.write(io, d.df, append=true, header=true)
    end;
end


"""
    interpolate!(d::Data)
    interpolate!(df::AbstractDataFrame)

In-place version of [`interpolate`](@ref).
"""
interpolate!(d::Data) = begin interpolate!(d.df); return d; end
function interpolate!(df::AbstractDataFrame)
    size(df)[2] >= 2 || error("interpolate needs at least two columns")
    df[!,1] .|> ismissing |> any && error("First column must not have missing values")

    for col in 2:size(df)[2]
        valid_row = 0
        for row in 1:size(df)[1]
            if ! ismissing(df[row, col])
                if (valid_row < row - 1) && (valid_row > 0)
                    # interpolate
                    t₀ = df[valid_row, 1]
                    v₀ = df[valid_row, col]
                    t₁ = df[row, 1]
                    v₁ = df[row, col]
                    # @show t₀ t₁ valid_row row v₀ v₁
                    for i in valid_row + 1 : row - 1
                        # @show i
                        df[i, col] = v₀ + (v₁ - v₀)*(df[i, 1] - t₀)/(t₁ - t₀)
                    end
                end
                valid_row = row
            end
        end
    end
end


"""
    interpolate(d::Data)
    interpolate(df::AbstractDataFrame)

Replace missing values with results of linear interpolation
performed against the first column (time).

```jldoctest
julia> x = DataFrame(t=[0.0, 1, 2, 3, 1000, 1001], v=[0.0, missing, missing, missing, 1000.0, missing])
6×2 DataFrame
 Row │ t        v
     │ Float64  Float64?
─────┼────────────────────
   1 │     0.0        0.0
   2 │     1.0  missing
   3 │     2.0  missing
   4 │     3.0  missing
   5 │  1000.0     1000.0
   6 │  1001.0  missing

julia> interpolate(x)
6×2 DataFrame
 Row │ t        v
     │ Float64  Float64?
─────┼────────────────────
   1 │     0.0        0.0
   2 │     1.0        1.0
   3 │     2.0        2.0
   4 │     3.0        3.0
   5 │  1000.0     1000.0
   6 │  1001.0  missing

```
"""
interpolate(d::Data) = Data(d::Data, interpolate(d.df))
function interpolate(df::AbstractDataFrame)
    d=deepcopy(df)
    interpolate!(d)
    return d
end

"""
    thermocam_correct!(d::Data)

Estimate correction for thermocamera temperatures and apply it. Return
the correction coefficients.

Correction is calculated from `CPU_0_temp` and `cam_cpu` columns.
This and the names of modified columns are currently hard coded.
"""
function thermocam_correct!(d::Data)
    di = interpolate(d.df)
    match_cols = [:CPU_0_temp, :cam_cpu]
    di′ = di[:, match_cols] |> dropmissing
    N = size(di′, 1)
    x = [di′[!, match_cols[2]] ones(N)] \ di′[!, match_cols[1]]
    cam_cols = [:cam_cpu, :cam_mem, :cam_board, :cam_table]
    d.df[!, cam_cols] .*= x[1]
    d.df[!, cam_cols] .+= x[2]
    x
end

# Function to fit: f(x) = T∞ + ∑ᵢ(kᵢ·e^(-x/τᵢ))

# - n is the system order, i.e., i ∈ 1:n
# - p = [ T∞, k₁, τ₁, k₂, τ₂, …, kₙ, τₙ]
function model(x, p)
    order = length(p) ÷ 2
    @. e(x, k, τ) = k*exp(-x/τ)
    p[1] .+ sum(i->e.(x, p[2i], p[2i+1]), 1:order)
end

# Jacobian of model to fit
function jacobian_model(x, p)
    order = length(p) ÷ 2
    J = Array{Float64}(undef, length(x), length(p))
    @. J[:,1] = 1                          # dmodel/dp[1]
    for i in 1:order
        p₂ = p[2i+0]
        p₃ = p[2i+1]
        @. J[:,2i+0] = exp(-x/p₃)           # dmodel/dp[2]
        @. J[:,2i+1] = p₂*x*exp(-x/p₃)/p₃^2 # dmodel/dp[3]
    end
    J
end

coef(res::CMPFit.Result) = res.param
rss(res::CMPFit.Result) = res.bestnorm
dof(res::CMPFit.Result) = res.dof
margin_error(res::CMPFit.Result) = res.perror
mse(res::CMPFit.Result) = rss(res)/dof(res)

"""
    printfit(fit; minutes = false)

Return the fitted function (result of [`fit`](@ref)) as Gnuplot
enhanced string. Time constants (τᵢ) are sorted from smallest to
largest.

# Example
```jldoctest
julia> f = Thermobench.fit(collect(0.0:10:50.0), [0, 6, 6.5, 6.8, 7, 7]);

julia> printfit(f)
"7.1 – 5.0⋅e^{−t/1.0} – 2.1⋅e^{−t/16.1}"
```
"""
function printfit(fit; minutes = false, mime = MIME"text/x-gnuplot")
    function print_exp(k, tau, mime)
        sign = " + "
        if k < 0
            sign = (mime == MIME"text/x-gnuplot") ? " – " : "-"
            k = -k
        end
        tau_str = @sprintf "%3.1f" tau
        if match(r"^[0.]*$", tau_str) != nothing
            tau_str = @sprintf "%3.2f" tau
        end
        if mime == MIME"text/x-gnuplot"
            sign * @sprintf("%3.1f⋅e^{−t/%s}", k, tau_str)
        elseif mime == MIME"text/latex"
            sign * @sprintf("%3.1f e^{\\frac{-t}{%s}}", k, tau_str)
        end
    end
    p = coef(fit)
    T∞ = p[1]
    τ = p[3:2:end] ./ (minutes ? 60 : 1)
    i = sortperm(τ)
    τ = τ[i]
    k = p[2:2:end][i]
    *(@sprintf("%3.1f", T∞), print_exp.(k, τ, mime)...)
end

@doc raw"""
    fit(
        time_s::Vector{Float64},
        data::Vector{Float64};
        order::Int64 = 2,
        p0 = nothing,
        tau_bounds = [(1, 60*60)],
        k_bounds = [(-120, 120)],
        T_bounds = (0, 120),
        use_cmpfit::Bool = true,
     )

Fit a thermal model to time series given by `time_s` and `data`. The
thermal model has the form of
```math
T(t) = T_∞ + \sum_{i=1}^{order}k_i⋅e^{-\frac{t}{τ_i}},
```
where T_∞, kᵢ and τᵢ are the coefficients found by this function.

If `use_cmpfit` is true (the default), use CMPFit.jl package rather
than LsqFit.jl. LsqFit doesn't work well in constrained settings.

You can limit the values of fitted parameters with `*_bounds`
parameters. Each bound is a tuple of lower and upper limit. `T_bounds`
limits the T∞ parameter. `tau_bounds` and `k_bounds` limit the
coefficients of exponential functions ``k·e^{-t/τ}``. If you specify
less tuples than the order of the model, the last limit will be
repeated.

# Example
```jldoctest
julia> using StatsBase: coef

julia> d = Thermobench.read("test.csv");


julia> f = fit(d.df.time, d.df.CPU_0_temp);


julia> coef(f)
5-element Array{Float64,1}:
  53.000281317694906
  -8.162698631078944
  59.36604041500533
 -13.124669051563407
 317.6295650259018

julia> printfit(f)
"53.0 – 8.2⋅e^{−t/59.4} – 13.1⋅e^{−t/317.6}"
```
"""
function fit(time_s::Vector{Float64}, data;
             order::Int64 = 2,
             p0 = nothing,
             tau_bounds::Vector{<:Tuple{Real,Real}} = [(1, 60*60)],
             k_bounds::Vector{<:Tuple{Real,Real}} = [(-120, 120)],
             T_bounds::Tuple{Real,Real} = (0, 120),
             attempts::Integer = 10,
             use_cmpfit::Bool = true,
             kwargs...)

    bounds = zeros(1 + 2*order, 2)
    bounds[1, :] = [T_bounds[1] T_bounds[2]] # p[1]: °C
    for i in 1:order
        bounds[2i, :] =  [k_bounds[min(i, length(k_bounds))]...] # p[2i]: °C
        bounds[2i + 1, :] = [tau_bounds[min(i, length(tau_bounds))]...] # p[2i+1]: seconds
    end
    lb = bounds[:,1]
    ub = bounds[:,2]
    df = DataFrame(time=time_s, data=data) |> dropmissing

    rng = MersenneTwister(length(time_s));

    if p0 === nothing
        p₀ = @. lb + (ub - lb)/2
        p₀[1] = clamp(df.data[end], lb[1], ub[1])
        for i in 1:order
            p₀[2i] = clamp((df.data[1] - df.data[end])/order, lb[2i], ub[2i])
        end
    elseif p0 === :random
        p₀ = @. lb + (ub - lb) * rand() # Default rng
    else
        p₀ = p0
    end
    let best_result, best_rss = Inf
        for attempt in 1:attempts
            if use_cmpfit == false
                # LsqFit
                try
                    result = LsqFit.curve_fit(model, jacobian_model,
                                              df.time, df.data, p₀; lower=lb, upper=ub,
                                              kwargs...)
                    if rss(result) < best_rss
                        best_result = result
                        best_rss = rss(result)
                    end
                    if stderror(best_result)[1] < 0.1
                        break
                    end
                catch e
                    @warn sprint(showerror, e)
                end
            else
                # CMPFit
                e = fill(0.5, size(df.data))
                pinfo = CMPFit.Parinfo(length(p₀))
                for i in 1:length(pinfo)
                    pinfo[i].limited = (1,1)
                    pinfo[i].limits = (lb[i], ub[i])
                    p₀[i] = clamp(p₀[i], lb[i], ub[i])
                    #@show pinfo[i]
                end
                best_result = CMPFit.cmpfit(df.time, df.data, e, model, p₀, parinfo=pinfo)
                break
            end
            # Another attempt with different initial solution (use local deterministic rng)
            p₀ = @. lb + (ub - lb) * rand(rng)
        end
        best_result
    end
end

ensurearray(x) = if isa(x, Array); x else [ x ] end

# Longest common prefix
# Source: https://rosettacode.org/wiki/Longest_common_prefix#Julia
function lcp(str::AbstractString...)
    r = IOBuffer()
    str = [str...]
    if !isempty(str)
        i = 1
        while all(i ≤ length(s) for s in str) &&
              all(s == str[1][i] for s in getindex.(str, i))
            print(r, str[1][i])
            i += 1
        end
    end
    return String(take!(r))
end

lcp(v::Vector{String}) = lcp(v...)

# Longest common suffix
function lcs(str::AbstractString...)
    r = ""
    str = [str...]
    if !isempty(str)
        i = 0
        while all(i < length(s) for s in str) &&
              all(s[end-i] == str[1][end-i] for s in str)
            r = str[1][end-i:end]
            i += 1
        end
    end
    return r
end

lcs(v::Vector{String}) = lcs(v...)

"Stores results of processing of one or more thermobench CSV files.

The main data is stored in the `result` field."
mutable struct MultiFit
    name::String
    result::DataFrame
    time                        # time range of all data
    subtract::Bool
end

Base.show(io::IO, mf::MultiFit) = begin
    println(io, typeof(mf), ": ", mf.name)
    print(io, "    ", select(mf.result, Not([:fit, :data, :series])))
end

"""
    rename!(mf::MultiFit, name)

Rename `MultiFit` data structure.

The name is often used as graph label so renaming can be used to set
descriptive graph labels.
"""
rename!(mf::MultiFit, name) = (mf.name = name; mf)

Base.filter(f, mf::MultiFit) = MultiFit(mf.name, filter(f, mf.result), mf.time, mf.subtract)

"""
    write(file, mf::MultiFit)

Write [`multi_fit()`](@ref) results to a CSV file `file`.
"""
function write(file, mf::MultiFit)
    CSV.write(file, select(mf.result, Not([:fit,:data,:series])))
end

"""
    plot(mf::MultiFit; kwargs...)::Vector{Gnuplot.PlotElement}

Plot MultiFit data.

# Arguments

- `minutes::Bool=true`: chooses between seconds and minutes on the
  x-axis.
- `pt_titles::Bool=true` whether to include titles of measured data
  points in separate legend column.
- `pt_decim::Int=1` draw only every `pt_decim`-th measured data point.
  This can be used to reduce the size of vector image formats.
- `pt_size::Real=1` size of measured data points.
- `pt_saturation::Real=0.4` intensity of points (0 is white, 1 is saturate)
- `stddev::Bool=true` whether to include root-mean-square error of the
  fit in the legend (as (±xxx)).
- `models::Bool=true` whether show thermal model expressions in the key.
"""
function plot(mf::MultiFit;
              minutes::Bool = true,
              pt_titles::Bool = true,
              models::Bool = true,
              pt_decim::Int = 1,
              pt_size::Real = 1,
              pt_saturation::Real = 0.4,
              stddev::Bool = true,
              )::Vector{Gnuplot.PlotElement}
    colors = distinguishable_colors(
        nrow(mf.result),
        [RGB(1,1,1)], dropseed=true)
    ptcolors = weighted_color_mean.(pt_saturation, colors, colorant"white")

    rmse = mf.result.rmse
    bad = 2 * quantile(rmse, 0.75)
    stddev_title(i) = """ {/*0.8 (±$(@sprintf "%.2g" rmse[i])$(rmse[i] > bad ? "{/:Bold*1.25 !!!}" : ""))}"""

    mf.result.name == mf.result.name[1]
    same_names = all(map(mf.result.name) do name name == mf.result.name[1] end)
    same_cols  = all(map(mf.result.column) do col col == mf.result.column[1] end)

    row_title(i) =
        ((!same_names || (same_names && same_cols)) ? mf.result.name[i] : "") *
        ((!same_names && !same_cols) ? ", " : "") *
        (same_cols  ? "" : String(mf.result.column[i])) *
        (!(same_names || same_cols) ? "$i" : "")
    pt_title(i) = pt_titles ? row_title(i) : ""
    fit_title(i) = (pt_titles ? "" : row_title(i) * ": ") *
        (models ? printfit(mf.result.fit[i], minutes=minutes) : "") *
        (stddev ? stddev_title(i) : "")


    time_unit = minutes ? "min" : "s"
    time_div = minutes ? 60 : 1

    vcat(
        Gnuplot.PlotElement(
            key="below left Left reverse horizontal maxcols $(pt_titles ? 2 : 1)",
            title= "$(mf.name)",
            xlabel="Time [$time_unit]", ylabel=(mf.subtract ? "Rel. temperature" : "Temperature") * " [°C]",
            cmds=["set grid", "set minussign"]),
        [
            Gnuplot.PlotElement(
                data=Gnuplot.DatasetText(mf.result.series[i].time[1:pt_decim:end]/time_div,
                                         mf.result.series[i].val[1:pt_decim:end]),
                plot="w p ps $pt_size lt $i lc rgb '#$(hex(ptcolors[i]))' title '$(pt_title(i))' noenhanced",
            )
            for i in 1:nrow(mf.result)]...,
        [
            Gnuplot.PlotElement(
                data=Gnuplot.DatasetText(mf.time/time_div, model(mf.time, coef(mf.result.fit[i]))),
                plot="w l lw 2 lc rgb '#$(hex(colors[i]))' title '$(fit_title(i))'",
            )
            for i in 1:nrow(mf.result)]...
    )
end

Gnuplot.recipe(mf::MultiFit) = plot(mf)

"""
    plot_bars(df::AbstractDataFrame; kwargs...)::Vector{Gnuplot.PlotElement}

Generic helper function to plot bar graphs with Gnuplot.jl.

# Arguments

- `df`: data to plot. The first column should contains text labels,
  the other columns the ploted values. If the values are of
  `Measurement` type, they will be plotted with errorbars style,
  unless overridden with `hist_style`.
- `box_width=0.8`: the width of the boxes. One means that the boxes
  touch each other.
- `gap::Union{Int64,Nothing}=nothing`: The gap between bar clusters.
  If `nothing`, it is set automatically depending on the number of
  bars in the cluster; to zero for one bar in the cluster, to 1 for
  multiple bars.
- `hist_style=nothing`: histogram style — see Gnuplot documentation.
- `fill_style="solid 1 border -1"`: fill style — see Gnuplot documentation.
- `errorbars=""`: errorbars style — see Gnuplot documentation.
- `label_rot=-30`: label rotation angle; if > 0, align label to right.
- `label_enhanced=false`: whether to apply Gnuplot enhanced formatting to labels.
- `key_enhanced=false`: whether to apply Gnuplot enhanced formatting to data keys.
- `y2cols=[]`: Columns (specified as symbols) which should be plot against *y2* axis.

# Example

```jldoctest
julia> using Measurements, Gnuplot

julia> df = DataFrame(names=["very long label", "b", "c"],
                      temp=10:12,
                      speed=collect(4:-1:2) .± 1)
3×3 DataFrame
 Row │ names            temp   speed
     │ String           Int64  Measurem…
─────┼───────────────────────────────────
   1 │ very long label     10    4.0±1.0
   2 │ b                   11    3.0±1.0
   3 │ c                   12    2.0±1.0

julia> @gp Thermobench.plot_bars(df)

```

"""
function plot_bars(df::AbstractDataFrame;
                   box_width = 0.8,
                   gap::Union{Int64,Nothing} = nothing,
                   hist_style = nothing,
                   fill_style = "solid 1 border -1",
                   errorbars = "",
                   label_rot = -30,
                   label_enhanced = false,
                   key_enhanced = false,
                   y2cols = [],
                   )::Vector{Gnuplot.PlotElement}
    n = nrow(df)

    foreach(y2cols) do col
        @assert col in propertynames(df)
    end

    axes(i) = (propertynames(df)[i] in y2cols) ? "axes x1y2 " : ""

    if hist_style === nothing
        hist_style = (any(map(eltype.(eachcol(df))) do type type <: Measurement end)
                      ? "errorbars" : "clustered")
    end
    eb = hist_style == "errorbars"

    if gap === nothing
        gap = ncol(df) == 2 ? 0 : 1
    end
    gap_str = (hist_style ∈ ["clustered", "errorbars"]) ? "gap $gap" : ""
    hist_style == "columnstacked" && @warn "columnstacked not well supported"

    gpusing(i) = begin
        cols = [i, "xticlabels(1)"]
        eb && insert!(cols, 2, i+ncol(df)-1)
        join(cols, ":")
    end

    data=Gnuplot.DatasetText(
        String.(df[!, 1]), # labels
        [Measurements.value.(df[!, i]) for i in 2:ncol(df)]...,
        [eltype(df[!, i]) <: Measurement ?
         Measurements.uncertainty.(df[!, i]) :
         fill(NaN, size(df[!, i]))
         for i in 2:ncol(df) if eb]...,
    )
    vcat(
        Gnuplot.PlotElement(
            xr=[-0.5,n-0.5],
            cmds=vcat(["set grid ytics y2tics",
                       "set style data histogram",
                       "set style histogram $hist_style $gap_str",
                       "set boxwidth $box_width",
                       "set style fill $fill_style",
                       """set xtics rotate by $label_rot $(label_rot > 0 ? "right" : "") $(label_enhanced ? "" : "no")enhanced""",
                       ],
                      !isempty(errorbars) ? ["set errorbars $errorbars"] : String[],
                      !isempty(y2cols) ? ["set ytics nomirror", "set y2tics"] : String[],
                      ),
        ),
        [
            Gnuplot.PlotElement(
                data=data,
                plot="using $(gpusing(i)) $(axes(i))" *
                """title '$(names(df, i)[1])' $(key_enhanced ? "" : "no")enhanced""",
            )
            for i in 2:ncol(df)
        ]...
    )
end

"""
    plot_Tinf(mfs::MultiFit...; kwargs...)::Vector{Gnuplot.PlotElement}

Plot ``T_∞`` as bargraphs. Multiple data sets can be passed as
arguments to compare them. `kwargs` are passed to
[`Thermobench.plot_bars`](@ref).
"""
function plot_Tinf(mfs::Vararg{MultiFit, N};
                   kwargs...)::Vector{Gnuplot.PlotElement} where {N}
    @assert length(mfs) > 0
    @assert all(mfs) do mf mfs[1].subtract == mf.subtract end "Same value of subtract"
    n = length(mfs[1].result.Tinf)
    @assert all(mfs) do mf nrow(mf.result) == n end "Same number of rows"

    name(i) = mfs[i].name != "" ? mfs[i].name : "$i"
    df = hcat(DataFrame(name=mfs[1].result.name),
              DataFrame([name(i) => mfs[i].result.Tinf for i in 1:length(mfs)],
                        makeunique=true))
    vcat(
        plot_bars(df; kwargs...)...,
        Gnuplot.PlotElement(ylabel=(mfs[1].subtract ? "Relative " : "") * " T_∞ [°C]",)
    )
end

"""
    plot_Tinf_and_ops(mf::MultiFit; kwargs...)::Vector{Gnuplot.PlotElement}

Plot ``T_∞`` and performance (ops per second from [`ops_est`](@ref))
as a bargraph.

# Arguments

- `perf_str`: Title to show for the performance legend and y-axis. Default is "Performance".
"""
function plot_Tinf_and_ops(mf::MultiFit;
                           perf_str="Performance",
                           kwargs...)::Vector{Gnuplot.PlotElement} where {N}
    df = DataFrame(name=mf.result.name,
                        T_∞=mf.result.Tinf,
                   Performace=mf.result.ops)
    rename!(df, :Performace => Symbol(perf_str))
    vcat(
        plot_bars(df; y2cols=[Symbol(perf_str)], key_enhanced=true, kwargs...)...,
        Gnuplot.PlotElement(ylabel=(mf.subtract ? "Relative " : "") * " T_∞ [°C]",
                            cmds=["set y2label '$perf_str [op/s]'"])
    )
end


"""
    multi_fit(sources, columns = :CPU_0_temp;
              name = nothing,
              timecol = :time,
              use_measurements = false,
              order::Int64 = 2,
              subtract = nothing,
              kwargs...)::MultiFit

Call [`fit()`](@ref) for all sources and report the results (coefficients
etc.) in `DataFrame`. When `use_measurements` is `true`, report
coefficients with their confidence intervals as `Measurement`
objects.

`subtract` specifies the column (symbol), which is subtracted from
data after interpolating its values with [`interpolate`](@ref). This
intended for subtraction of ambient temperature.

```jldoctest
julia> multi_fit("test.csv", [:CPU_0_temp :CPU_1_temp])
Thermobench.MultiFit: test.csv
    2×9 DataFrame
 Row │ name      column      rmse       ops                Tinf     k1         ⋯
     │ String    Symbol      Float64    Measurem…          Float64  Float64    ⋯
─────┼──────────────────────────────────────────────────────────────────────────
   1 │ test.csv  CPU_0_temp   0.308966  3.9364e8±280000.0  53.0003  -8.1627    ⋯
   2 │ test.csv  CPU_1_temp  14.1697    3.9364e8±280000.0  53.7449  -8.79378
                                                               3 columns omitted
```
"""
function multi_fit(sources, columns = :CPU_0_temp;
                   name = nothing,  # label for plotting
                   timecol = :time,
                   use_measurements = false,
                   order::Int64 = 2,
                   subtract = nothing, # column to subtract
                   kwargs...)::MultiFit

    type = use_measurements ? Measurement{Float64} : Float64

    function coef2df(coef)
        order = length(coef) ÷ 2
        hcat(DataFrame(Tinf = coef[1]),
             [DataFrame("k$i" => coef[2i], "tau$i" => coef[2i+1]) for i in 1:order]...)
    end

    result = hcat(DataFrame(name = String[],
                            column = Symbol[],
                            rmse = Float64[], # root-mean-square error
                            ops = Measurement[], # work_done ⇒ oper. per second
                            ),
                  coef2df(fill(type[], 2order+1)),
                  DataFrame(fit = Any[],
                            data = Data[],
                            series = DataFrame[]))

    tmin = +Inf
    tmax = -Inf

    for source in ensurearray(sources)
        d = try
            convert(Data, source)
        catch e
            read(source)
        end
        sub = (subtract != nothing) ? interpolate(d.df[!, [timecol, subtract]])[!,2] : 0
        for col in ensurearray(columns)
            series = DataFrame(time = d.df[!, timecol], val = d.df[!, col] .- sub) |> dropmissing
            tmin = min(tmin, first(series.time))
            tmax = max(tmax, last(series.time))
            f = fit(series.time, series.val; order=order, kwargs...)

            coefs = coef(f)

            # Sort coefs by increasing τ
            τ = coefs[3:2:end]
            idx = sortperm(τ)
            coefs[2:2:end] = coefs[2idx.+0]
            coefs[3:2:end] = coefs[2idx.+1]

            local mes
            if use_measurements
                mes = margin_error(f)
                mes[2:2:end] = mes[2idx.+0]
                mes[3:2:end] = mes[2idx.+1]
            end

            append!(result,
                    hcat(DataFrame(name = d.name,
                                   column = col,
                                   rmse = sqrt(mse(f)),
                                   ops = ops_est(d),
                                   fit = f,
                                   data = d,
                                   series = series),
                         coef2df(use_measurements ? measurement.(coefs, mes) : coefs)))
        end
    end

    if name === nothing
        name = lcp(result.name)
    end

    MultiFit(name, result, range(tmin, tmax, length=500), subtract != nothing)
end

"""
Construct array of symbols from arguments.

Useful for constructing column names, e.g.,
```jldoctest
julia> @symarray Cortex_A57_temp Denver2_temp
2-element Array{Symbol,1}:
 :Cortex_A57_temp
 :Denver2_temp
```
"""
macro symarray(sym...)
    return [sym...]
end

"""
    plot_fit(sources, columns = :CPU_0_temp;
             timecol = :time,
             kwargs...)

Call [`fit`](@ref) for all `sources` and `columns` and produce a graph
using gnuplot.

!!! note

    This function is now superseded by [`multi_fit`](@ref), which
    offers more possibilities for plotting the data.

`sources` can be a file name (`String`) or a `DataFrame` or an array
of these.

`timecol` is the columns with time of measurement.

Setting `plotexp` to `true` causes the individual fitted exponentials to
be plotted in addition to the compete fitted function.

Other `kwargs` are passed to [`fit`](@ref).

# Example
```jldoctest
julia> plot_fit("test.csv", [:CPU_0_temp :CPU_1_temp], order=2);


```
"""
function plot_fit(sources, columns = :CPU_0_temp;
                  timecol = :time,
                  plotexp = false,
                  ambient = nothing,
                  kwargs...)
    local prefix = ""
    if isa(sources, Array) && eltype(sources) <: AbstractString
        prefix = lcp(sources...)
    elseif typeof(sources) <: AbstractString
        prefix = sources
    end
    @gp("set grid", "set minussign",
        "set key below left Left reverse horizontal maxcols 2",
        "set title '$prefix*'",
        "set xlabel 'Time [min]'", "set ylabel 'Temperature [°C]'",
        :-)
    if ambient != nothing
        @gp :- "set multiplot layout 2,1" 1 :-
    end
    gnuplot_escape(s) = replace(s, r"([_^@&~])" => s"\\\1")
    colors = distinguishable_colors(
        length(ensurearray(sources)) * length(ensurearray(columns)),
        [RGB(1,1,1)], dropseed=true)
    local fit
    for plot in (:points, :fit)
        local plotno = 1
        for source in ensurearray(sources)
            df = if isa(source, AbstractString); read(source).df else source.df end
            for col in ensurearray(columns)
                color = colors[(plotno - 1) % length(colors) + 1]
                series = DataFrame(time = df[!, timecol], val = df[!, col]) |> dropmissing
                if plot == :points
                    if isa(source, AbstractString)
                        title = source[1+length(prefix):end]
                        if length(title) > 0; title = "*"*title*": " end
                    else
                        title = ""
                    end
                    ptcolor = weighted_color_mean(0.4, color, colorant"white")
                    @gp(:-, 1,
                        series.time./60, series.val,
                        "w p ps 1 lt $plotno lc rgb '#$(hex(ptcolor))' title '$title$(String(col))' noenhanced",
                        :-)
                    if ambient != nothing
                        if ambient === true
                            ambient = :ambient
                        end
                        series = DataFrame(time = df[!, timecol], amb = df[!, ambient]) |> dropmissing
                        @gp(:-, 2, title="Ambient temperature",
                            series.time./60, series.amb,
                            "w l lt $plotno lc rgb '#$(hex(color))' title '$title' noenhanced",
                        :-)
                    end
                end
                if plot == :fit
                    t₀ = series.time[1]
                    x = range(t₀, series.time[end], length=min(length(series.time), 1000))
                    fit = Thermobench.fit(series[!, :time] .- t₀, series.val; kwargs...)
                    @gp(:-, 1, x./60, model(x .- t₀, coef(fit)),
                        "w l lt $plotno lc rgb '#$(hex(color))' lw 2 title '$(printfit(fit, minutes=true))'",
                        :-)
                    if plotexp
                        expcolor = weighted_color_mean(0.4, color, colorant"white")
                        local c = coef(fit)
                        for i in 1:length(c)÷2
                            cc = zeros(size(c))
                            idx = [1, 2i, 2i+1]
                            cc[idx] = c[idx]
                            @gp(:-, 1, x ./ 60, model(x .- t₀, cc),
                                """w l lt $plotno lc rgb '#$(hex(expcolor))' lw 1 title 'exp τ=$(@sprintf("%4.2f", cc[2i+1]/60))'""",
                                :-)
                        end
                    end
                end
                plotno += 1
            end
        end
    end
    fig = @gp()
    return fit
end

end # module
