module Thermobench

import CSV
import DataFrames: DataFrame, AbstractDataFrame, rename!, dropmissing, Not, select, nrow
using Printf, Gnuplot, Colors, Random
import LsqFit: margin_error, mse
import LsqFit
import CMPFit
import StatsBase: coef, dof, nobs, rss, stderror, weights, residuals
import Statistics: quantile
using Measurements
using Debugger

export
    @symarray,
    fit,
    interpolate!,
    interpolate,
    multi_fit,
    plot_fit,
    printfit

Base.endswith(sym::Symbol, str::AbstractString) = endswith(String(sym), str)

"Normalize units to seconds and °C"
function normalize_units!(df::AbstractDataFrame)
    for col in propertynames(df)
        if col == :time_ms
            df[!, col] ./= 1000
            rename!(df, col => "time_s")
        elseif endswith(col, "_m°C")
            df[!, col] ./= 1000
            rename!(df, col => replace(String(col), r"_m°C$" => "_°C"))
        elseif endswith(col, "_mC") # Older thermobench version
            df[!, col] ./= 1000
            rename!(df, col => replace(String(col), r"_mC$" => "_°C"))
        end
    end
end

"""
    read(source; normalizeunits=true)

Read thermobech CSV file, optionally normalizing units and return it
as DataFrame"
"""
function read(source; normalizeunits=true)
    df = CSV.read(source; comment="#", normalizenames=true,
                  silencewarnings=true, copycols=true,
                  typemap=Dict(Int64 => Float64), # Needed for interpolation
                  )
    if normalizeunits
        normalize_units!(df)
    end
    return df
end

"""
    interpolate!(df::AbstractDataFrame)

In-place version of [`interpolate`](@ref).
"""
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
    interpolate!(df::AbstractDataFrame)

Replace missing values with results of linear interpolation
performed against the first column (time).

```jldoctest
julia> x = DataFrame(t=[0.0, 1, 2, 3, 1000, 1001], v=[0.0, missing, missing, missing, 1000.0, missing])
6×2 DataFrame
│ Row │ t       │ v        │
│     │ Float64 │ Float64? │
├─────┼─────────┼──────────┤
│ 1   │ 0.0     │ 0.0      │
│ 2   │ 1.0     │ missing  │
│ 3   │ 2.0     │ missing  │
│ 4   │ 3.0     │ missing  │
│ 5   │ 1000.0  │ 1000.0   │
│ 6   │ 1001.0  │ missing  │

julia> interpolate(x)
6×2 DataFrame
│ Row │ t       │ v        │
│     │ Float64 │ Float64? │
├─────┼─────────┼──────────┤
│ 1   │ 0.0     │ 0.0      │
│ 2   │ 1.0     │ 1.0      │
│ 3   │ 2.0     │ 2.0      │
│ 4   │ 3.0     │ 3.0      │
│ 5   │ 1000.0  │ 1000.0   │
│ 6   │ 1001.0  │ missing  │

```
"""
function interpolate(df::AbstractDataFrame)
    d=deepcopy(df)
    interpolate!(d)
    return d
end

"""
    thermocam_correct!(df::AbstractDataFrame)

Estimate correction for thermocamera temperatures and apply it. Return
the correction coefficients.

Correction is calculated from `CPU_0_temp_°C` and `cam_cpu` columns.
This and the names of modified columns are currently hard coded.
"""
function thermocam_correct!(df::AbstractDataFrame)
    di = interpolate(df)
    match_cols = [:CPU_0_temp_°C, :cam_cpu]
    d = di[:, match_cols] |> dropmissing
    N = size(d, 1)
    x = [d[!, match_cols[2]] ones(N)] \ d[!, match_cols[1]]
    cam_cols = [:cam_cpu, :cam_mem, :cam_board, :cam_table]
    df[!, cam_cols] .*= x[1]
    df[!, cam_cols] .+= x[2]
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

Return the fitted function as Gnuplot enhanced string. Time constants
(τᵢ) are sorted from smallest to largest.
"""
function printfit(fit; minutes = false)
    function print_exp(k, tau)
        sign = " + "
        if k < 0
            sign = " – "
            k = -k
        end
        tau_str = @sprintf "%3.1f" tau
        if match(r"^[0.]*$", tau_str) != nothing
            tau_str = @sprintf "%3.2f" tau
        end
        sign * @sprintf("%3.1f⋅e^{−t/%s}", k, tau_str)
    end
    p = coef(fit)
    T∞ = p[1]
    τ = p[3:2:end] ./ (minutes ? 60 : 1)
    i = sortperm(τ)
    τ = τ[i]
    k = p[2:2:end][i]
    *(@sprintf("%3.1f", T∞), print_exp.(k[i], τ[i])...)
end

"""
    fit(
        time_s::Vector{Float64},
        data;
        order::Int64 = 2,
        p0 = nothing,
        tau_bounds = [(1, 60*60)],
        k_bounds = [(-120, 120)],
        T_bounds = (0, 120),
        use_cmpfit::Bool = false,
     )

Fit a thermal model to time series.

If `use_cmpfit` is true, use CMPFit.jl package rather than LsqFit.jl.
LsqFit doesn't work well in constrained fit.

You can limit the values of fitted parameters with `*_bounds`
parameters. Each bound is a tuple of lower and upper limit. `T_bounds`
limits the T∞ parameter. `tau_bounds` and `k_bounds` limit the
coefficients of exponential functions ``k·e^{-t/τ}``. If you specify
less tuples than the order of the model, the last limit will be
repeated.

# Example
```julia
df = read("test.csv")
f = fit(df.time_s, df.CPU_0_temp_°C)
coef(f)
Thermobench.printfit(f)
```
"""
function fit(time_s::Vector{Float64}, data;
             order::Int64 = 2,
             p0 = nothing,
             tau_bounds::Vector{<:Tuple{Real,Real}} = [(1, 60*60)],
             k_bounds::Vector{<:Tuple{Real,Real}} = [(-120, 120)],
             T_bounds::Tuple{Real,Real} = (0, 120),
             attempts::Integer = 10,
             use_cmpfit::Bool = false,
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
            p₀[2i] = clamp(df.data[1] - df.data[end], lb[2i], ub[2i])
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

mutable struct MultiFit
    prefix::String
    data::Vector{DataFrame}
    result::DataFrame
    time
    subtract::Bool
end

Base.show(io::IO, mf::MultiFit) = begin
    println(io, "Prefix: ", mf.prefix)
    print(io, select(mf.result, Not([:fit])))
end

function plot_mf(mf::MultiFit;
                 minutes::Bool = true,
                 pt_titles::Bool = true,
                 pt_decim::Int = 1,
                 pt_size::Real = 1,
                 stddev::Bool = true,
                 )::Vector{Gnuplot.PlotElement}
    colors = distinguishable_colors(
        nrow(mf.result),
        [RGB(1,1,1)], dropseed=true)
    ptcolors = weighted_color_mean.(0.4, colors, colorant"white")

    stderr = sqrt.(mf.result.mse)
    bad = 2 * quantile(stderr, 0.90)
    stddev_title(i) = """ {/*0.8 (±$(@sprintf "%.2g" stderr[i])$(stderr[i] > bad ? "{/:Bold*1.25 !!!}" : ""))}"""
    pt_title(i) = pt_titles ? mf.result.name[i] : ""
    fit_title(i) = (pt_titles ? "" : mf.result.name[i] * ": ") *
        printfit(mf.result.fit[i], minutes=minutes) *
        (stddev ? stddev_title(i) : "")


    time_unit = minutes ? "min" : "s"
    time_div = minutes ? 60 : 1

    vcat(
        Gnuplot.PlotElement(
            key="below left Left reverse horizontal maxcols $(pt_titles ? 2 : 1)",
            title= "$(mf.prefix)*",
            xlabel="Time [$time_unit]", ylabel=(mf.subtract ? "Rel. temperature" : "Temperature") * " [°C]",
            cmds=["set grid", "set minussign"]),
        [
            Gnuplot.PlotElement(
                data=Gnuplot.DatasetText(mf.data[i].time[1:pt_decim:end]/time_div,
                                         mf.data[i].val[1:pt_decim:end]),
                plot="w p ps $pt_size lt $i lc rgb '#$(hex(ptcolors[i]))' title '$(pt_title(i))'",
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

Gnuplot.recipe(mf::MultiFit) = plot_mf(mf)

"""
    multi_fit(sources, columns = :CPU_0_temp_°C;
              timecol = :time_s,
              use_measurements = false,
              order::Int64 = 2,
              subtract = nothing,
              kwargs...)::MultiFit

Call `fit()` for all sources and report the results (coefficients
etc.) in `DataFrame`. When `use_measurements` is `true`, report
coefficients with their confidence intervals as `Measurement`
objects.

`subtract` specifies the column (symbol), which is subtracted from
data after interpolating its values with [`interpolate`](@ref). This
intended for subtraction of ambient temperature.

```jldoctest
julia> multi_fit("test.csv", [:CPU_0_temp_°C :CPU_1_temp_°C])
Prefix: test.csv
2×8 DataFrame
│ Row │ name   │ column        │ mse       │ Tinf    │ k1       │ tau1    │ k2       │ tau2    │
│     │ String │ Symbol        │ Float64   │ Float64 │ Float64  │ Float64 │ Float64  │ Float64 │
├─────┼────────┼───────────────┼───────────┼─────────┼──────────┼─────────┼──────────┼─────────┤
│ 1   │        │ CPU_0_temp_°C │ 0.023865  │ 53.0003 │ -8.1627  │ 59.366  │ -13.1247 │ 317.63  │
│ 2   │        │ CPU_1_temp_°C │ 0.0208397 │ 54.0527 │ -7.17072 │ 51.1449 │ -14.3006 │ 277.687 │
```
"""
function multi_fit(sources, columns = :CPU_0_temp_°C;
                   timecol = :time_s,
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

    data = Vector{DataFrame}()
    result = hcat(DataFrame(name = String[],
                            column = Symbol[],
                            mse = Float64[]),
                  coef2df(fill(type[], 2order+1)),
                  DataFrame(fit = Any[]))

    local prefix = ""
    if isa(sources, Array) && eltype(sources) <: AbstractString
        prefix = lcp(sources...)
    elseif typeof(sources) <: AbstractString
        prefix = sources
    end

    tmin = +Inf
    tmax = -Inf

    for source in ensurearray(sources)
        df, name = if isa(source, AbstractString)
            (read(source), source[1+length(prefix):end])
        elseif isa(source, Tuple)
            (source[1], source[2])
        else
            (source, "")
        end
        sub = (subtract != nothing) ? interpolate(df[!, [timecol, subtract]])[!,2] : 0
        for col in ensurearray(columns)
            series = DataFrame(time = df[!, timecol], val = df[!, col] .- sub) |> dropmissing
            push!(data, series)
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
                    hcat(DataFrame(name = name,
                                   column = col,
                                   mse = mse(f),
                                   fit = f),
                         coef2df(use_measurements ? measurement.(coefs, mes) : coefs)))
        end
    end
    MultiFit(prefix, data, result, range(tmin, tmax, length=1000), subtract != nothing)
end

"""
Construct array of symbols from arguments.

Useful for constructing column names, e.g.,
```julia
@symarray Cortex_A57_temp_°C Denver2_temp_°C
```
"""
macro symarray(sym...)
    return [sym...]
end
"""
    plot_fit(sources, columns = :CPU_0_temp_°C;
             timecol = :time_s,
             kwargs...)

Call [`fit`](@ref) for all `sources` and `columns` and produce a graph
using gnuplot.

`sources` can be a file name (`String`) or a `DataFrame` or an array
of these.

`timecol` is the columns with time of measurement.

Setting `plotexp` to `true` causes the individual fitted exponentials to
be plotted in addition to the compete fitted function.

Other `kwargs` are passed to [`fit`](@ref).

# Example
```julia
plot_fit(
    ["file\$i.csv" for i in 1:3],
    [:CPU_0_temp_°C, :GPU_0_temp_°C],
    order = 2
)
```

```jldoctest
julia> plot_fit("test.csv", [:CPU_0_temp_°C :CPU_1_temp_°C])
rss(fit) = 16.824847736915736
rss(fit) = 14.692012083459852
LsqFit.LsqFitResult{Array{Float64,1},Array{Float64,1},Array{Float64,2},Array{Float64,1}}([54.05270193617456, -14.300579425822987, 277.6871572171704, -7.170707187492722, 51.1449427363974], [0.4814153228588438, 0.48210393166134935, -0.02827469472873645, 0.1590733915246858, -0.25639810428151577, -0.2745094012142104, -0.29552833686121005, -0.3190430919820457, 0.054645980053877, -0.1740621999878087  …  -0.10112334959897851, -0.09697970488952024, -0.09284269210126439, 0.11127121083011815, 0.11537443369089573, 0.11946698541304812, -0.0764592578095602, -0.07240014726207278, 0.1316443697488907, -0.26432565427360544], [1.0 1.0 … 1.0 -0.0; 1.0 0.9999870640586986 … 0.9999297674088051 -9.84651920490284e-6; … ; 1.0 0.07839196015996269 … 9.920656343511463e-7 -1.9227241374458278e-6; 1.0 0.07811016470837553 … 9.728568932023543e-7 -1.8881625354797446e-6], true, Float64[])
```
"""
function plot_fit(sources, columns = :CPU_0_temp_°C;
                  timecol = :time_s,
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
            df = if isa(source, AbstractString); read(source) else source end
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
                            ambient = :ambient_°C
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
                    @show rss(fit) #fit.converged
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
