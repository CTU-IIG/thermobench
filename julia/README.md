
<a id='Thermobench.jl-1'></a>

# Thermobench.jl




Julia module for working with `thermobench`-produced CSV files.


<a id='Installation-1'></a>

## Installation


Run `julia` and type:


```julia
]develop /path/to/thermobench/julia
```


<a id='Reference-1'></a>

## Reference

<a id='Thermobench.fit-Tuple{Array{Float64,1},Any}' href='#Thermobench.fit-Tuple{Array{Float64,1},Any}'>#</a>
**`Thermobench.fit`** &mdash; *Method*.



```julia
fit(
    time_s::Vector{Float64},
    data;
    order::Int64 = 2,
    p0 = nothing,
    tau_bounds = [(1, 60*60)],
 )
```

Fit a thermal model to time series.

**Example**

```julia
df = read("test.csv")
f = fit(df.time_s, df.CPU_0_temp_°C)
coef(f)
Thermobench.printfit(f)
```


<a target='_blank' href='https://github.com/CTU-IIG/thermobench/blob/8b1ba38de1015384cfde0afc463a973f058b356b/julia/src/funcs.jl#L157-L175' class='documenter-source'>source</a><br>

<a id='Thermobench.interpolate!-Tuple{DataFrames.AbstractDataFrame}' href='#Thermobench.interpolate!-Tuple{DataFrames.AbstractDataFrame}'>#</a>
**`Thermobench.interpolate!`** &mdash; *Method*.



```julia
interpolate!(df::AbstractDataFrame)
```

In-place version of [`interpolate`](index.md#Thermobench.interpolate-Tuple{DataFrames.AbstractDataFrame}).


<a target='_blank' href='https://github.com/CTU-IIG/thermobench/blob/8b1ba38de1015384cfde0afc463a973f058b356b/julia/src/funcs.jl#L33-L37' class='documenter-source'>source</a><br>

<a id='Thermobench.interpolate-Tuple{DataFrames.AbstractDataFrame}' href='#Thermobench.interpolate-Tuple{DataFrames.AbstractDataFrame}'>#</a>
**`Thermobench.interpolate`** &mdash; *Method*.



```julia
interpolate!(df::AbstractDataFrame)
```

Replace missing values with results of linear interpolation performed against the first column (time).

```julia-repl
julia> x = DataFrame(t=[0.0, 1, 2, 3, 1000], v=[0.0, missing, missing, missing, 1000.0])
5×2 DataFrames.DataFrame
│ Row │ t       │ v        │
│     │ Float64 │ Float64? │
├─────┼─────────┼──────────┤
│ 1   │ 0.0     │ 0.0      │
│ 2   │ 1.0     │ missing  │
│ 3   │ 2.0     │ missing  │
│ 4   │ 3.0     │ missing  │
│ 5   │ 1000.0  │ 1000.0   │

julia> interpolate(x)
5×2 DataFrames.DataFrame
│ Row │ t       │ v        │
│     │ Float64 │ Float64? │
├─────┼─────────┼──────────┤
│ 1   │ 0.0     │ 0.0      │
│ 2   │ 1.0     │ 1.0      │
│ 3   │ 2.0     │ 2.0      │
│ 4   │ 3.0     │ 3.0      │
│ 5   │ 1000.0  │ 1000.0   │

```


<a target='_blank' href='https://github.com/CTU-IIG/thermobench/blob/8b1ba38de1015384cfde0afc463a973f058b356b/julia/src/funcs.jl#L65-L95' class='documenter-source'>source</a><br>

<a id='Thermobench.plot_fit' href='#Thermobench.plot_fit'>#</a>
**`Thermobench.plot_fit`** &mdash; *Function*.



```julia
plot_fit(sources, columns = :CPU_0_temp_°C;
                  kwargs...)
```

Call [`fit`](index.md#Thermobench.fit-Tuple{Array{Float64,1},Any}) for all `sources` and `columns` and produce a graph using gnuplot.

`sources` can be a file name (`String`) or a `DataFrame` or an array of these.

**Example**

```julia
plot_fit(
    ["file$i.csv" for i in 1:3],
    [:CPU_0_temp_°C, :GPU_0_temp_°C],
    order = 2
)
```


<a target='_blank' href='https://github.com/CTU-IIG/thermobench/blob/8b1ba38de1015384cfde0afc463a973f058b356b/julia/src/funcs.jl#L253-L271' class='documenter-source'>source</a><br>

<a id='Thermobench.printfit-Tuple{Any}' href='#Thermobench.printfit-Tuple{Any}'>#</a>
**`Thermobench.printfit`** &mdash; *Method*.



```julia
printfit(fit; minutes = false)
```

Return the fitted function as Gnuplot enhanced string. Time constants (τᵢ) are sorted from smallest to largest.


<a target='_blank' href='https://github.com/CTU-IIG/thermobench/blob/8b1ba38de1015384cfde0afc463a973f058b356b/julia/src/funcs.jl#L124-L129' class='documenter-source'>source</a><br>

<a id='Thermobench.@symarray-Tuple' href='#Thermobench.@symarray-Tuple'>#</a>
**`Thermobench.@symarray`** &mdash; *Macro*.



Construct array of symbols from arguments.

Useful for constructing column names, e.g.,

```julia
@symarray Cortex_A57_temp_°C Denver2_temp_°C
```


<a target='_blank' href='https://github.com/CTU-IIG/thermobench/blob/8b1ba38de1015384cfde0afc463a973f058b356b/julia/src/funcs.jl#L242-L249' class='documenter-source'>source</a><br>

<a id='Thermobench.normalize_units!-Tuple{DataFrames.AbstractDataFrame}' href='#Thermobench.normalize_units!-Tuple{DataFrames.AbstractDataFrame}'>#</a>
**`Thermobench.normalize_units!`** &mdash; *Method*.



Normalize units to seconds and °C


<a target='_blank' href='https://github.com/CTU-IIG/thermobench/blob/8b1ba38de1015384cfde0afc463a973f058b356b/julia/src/funcs.jl#L1' class='documenter-source'>source</a><br>



