module Thermobench

import CSV
import DataFrames: DataFrame, AbstractDataFrame, rename!, dropmissing, Not, select
using Printf, Gnuplot, Colors, Random
import LsqFit: margin_error, mse
import LsqFit
import CMPFit
import StatsBase: coef, dof, nobs, rss, stderror, weights, residuals

export
    @symarray,
    fit,
    interpolate!,
    interpolate,
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

```jldoctest; setup = :(using DataFrames, Thermobench)
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
    p = coef(fit)
    T∞ = p[1]
    τ = p[3:2:end] ./ (minutes ? 60 : 1)
    i = sortperm(τ)
    τ = τ[i]
    k = p[2:2:end][i]
    join([@sprintf("%4.2f", T∞);
          [@sprintf("%4.2f⋅e^{−t/%4.2f}", k[i], τ[i]) for i in 1:length(τ)]],
         " + ")
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
             tau_bounds::Vector{Tuple{T, T}} = [(1, 60*60)],
             k_bounds::Vector{Tuple{T, T}} = [(-120, 120)],
             T_bounds::Tuple{T, T} = (0, 120),
             attempts::Integer = 10,
             use_cmpfit::Bool = false,
             kwargs...) where {T<:Real}

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
        p₀[1] = df.data[end]
        for i in 1:order
            p₀[2i] = (df.data[1] - df.data[end])
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
"""
function plot_fit(sources, columns = :CPU_0_temp_°C;
                  timecol = :time_s,
                  plotexp = false,
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
                    color = weighted_color_mean(0.4, color, colorant"white")
                    @gp(:-,
                        series.time./60, series.val,
                        "w p ps 1 lt $plotno lc rgb '#$(hex(color))' title '$title$(String(col))' noenhanced",
                        :-)
                end
                if plot == :fit
                    t₀ = series.time[1]
                    x = range(t₀, series.time[end], length=min(length(series.time), 1000))
                    fit = Thermobench.fit(series[!, :time] .- t₀, series.val; kwargs...)
                    @gp(:-, x./60, model(x .- t₀, coef(fit)),
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
                            @gp(:-, x ./ 60, model(x .- t₀, cc),
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
