# Thermobench.jl

```@meta
DocTestSetup = :(using Thermobench, DataFrames)
```

```@setup abc
using Gnuplot, Pipe, DataFramesMeta
Gnuplot.quitall()
mkpath("build/assets")
Gnuplot.options.term = "unknown"
empty!(Gnuplot.options.init)
push!(Gnuplot.options.init, linetypes(:Set1_5, lw=1.5, ps=1.5))
push!(Gnuplot.options.init, "set errorbars lw 2")
saveas(file; width=800, height=350) = save(term="pngcairo size $width,$height fontscale 0.8", output="build/assets/$(file).png")
```

Julia module for working with
[thermobench](https://github.com/CTU-IIG/thermobench)-produced CSV
files.

## Contents

```@contents
Depth = 4
```


## Installation

Thermobench.jl can be installed using the Julia package manager. From the Julia REPL, type `]` to enter the Pkg REPL mode and run:

```
pkg> add https://github.com/CTU-IIG/thermobench:julia
```

Alternatively, if you want to use the same versions of packages as the
author of the package, run:

```
(@v1.6) pkg> activate /path/to/thermobench/julia
(Thermobench) pkg> instantiate
```

## Usage

Thermobench package supports plotting with
[Gnuplot.jl](https://github.com/gcalderone/Gnuplot.jl) package so you
will most likely need both packages:

```@example abc
using Thermobench, Gnuplot
```

You can also create a shortcut `T` for accessing non-exported members
of Thermobench package.

```@example abc
const T = Thermobench
nothing # hide
```

### High-level data processing and graphing

The simplest way to using the package is the [`multi_fit`](@ref)
function. In the example below, it reads the data from a CSV file and
fits a thermal model to it. The result can be directly plotted by Gnuplot.jl:

```@repl abc
@gp multi_fit("test1.csv")
saveas("test-mf") # hide
```
![](assets/test-mf.png)

You can pass multiple CSV files to [`multi_fit`](@ref). The result is
shown as a DataFrame, which makes it easy to compare the results. You
can also specify additional keywords to control the operation. Below
we use `subtract` to subtract ambient temperature from the data to
fit, `use_measurements` to produce results with confidence intervals
and `use_cmpfit` to use alternative fitting solver.

```@repl abc
mf = multi_fit(["test1.csv", "test2.csv"], subtract=:ambient, use_cmpfit=true, use_measurements=true)
@gp mf
saveas("test-mf2") # hide
```
![](assets/test-mf2.png)

In most cases, we are interested only in ``T_∞`` parameters. These can
be plot (and compared between multiple data sets) with [`plot_Tinf`](@ref).

```@repl abc
mf2 = multi_fit(["test1.csv", "test2.csv"], :CPU_1_temp, name="CPU1", subtract=:ambient, use_cmpfit=true, use_measurements=true)
@gp plot_Tinf(rename!(mf, "CPU0"), mf2) key="left"
saveas("tinf", width=400) # hide
```
![](assets/tinf.png)

Both ``T_∞`` and benchmark performance can be plotted with [`plot_Tinf_and_ops`](@ref):

```@repl abc
@gp plot_Tinf_and_ops(mf2) key="left"
saveas("tinf-ops", width=480) # hide
```
![](assets/tinf-ops.png)


### Raw thermobench data

To access raw data from thermobench CSV files, use the [`Thermobench.read`](@ref)
function:

```@repl abc
using DataFrames
d = T.read("test1.csv");
dump(d, maxdepth=1)
first(d.df, 6)
```

In the example above, the data is available in `d.df` DataFrame, where
you can manipulate them as you want. You can find a lot of examples in
[DataFrames.jl
documentation](https://dataframes.juliadata.org/stable/).

To write (possibly modified) data to a file, use
[`Thermobench.write`](@ref) function.

#### Plotting

You can plot the data by using directly the values from DataFrame
`d.df`, but the [`plot(::Thermobench.Data)`](@ref) method
makes it easier:

```@example abc
@gp    plot(d, :CPU_0_temp) key="left"
@gp :- plot(d, :ambient, with="lines", title="Amb. temperature")
saveas("raw-cpu") # hide
```
![](assets/raw-cpu.png)

#### Missing values and interpolation

If you want to get rid of missing data, you can select interesting
columns and pass the dataframe through [`dropmissing`](https://dataframes.juliadata.org/stable/lib/functions/#DataFrames.dropmissing):

```@repl abc
select(d.df, [:time, :CPU_0_temp]) |> dropmissing
```
which is the same as:
```@repl abc
dropmissing(select(d.df, [:time, :CPU_0_temp]));
```
or
```@repl abc
using Pipe: @pipe
@pipe d.df |> select(_, [:time, :CPU_0_temp]) |> dropmissing(_);
```

Note that when you select all columns, you will likely end up with
empty dataframe, because `dropmissing` keeps only rows with **no**
missing values.

Alternatively, you can get rid of missing data by interpolating them
with [`interpolate`](@ref):

```@repl abc
interpolate(d)
```

#### Other useful data manipulations

To filter out some rows, you can use:

```@repl abc
using DataFramesMeta
@linq d.df |> where(10.0 .< :time .< 13.0)
```

## Reference

```@index
```

```@autodocs
Modules = [Thermobench]
```

```@meta
#DocTestSetup = nothing
```
