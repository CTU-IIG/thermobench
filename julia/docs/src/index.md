# Thermobench.jl

```@meta
DocTestSetup = :(using Thermobench, DataFrames)
```

```@setup abc
using Gnuplot
Gnuplot.quitall()
mkpath("build/assets")
Gnuplot.options.term = "unknown"
empty!(Gnuplot.options.init)
push!( Gnuplot.options.init, linetypes(:Set1_5, lw=1.5, ps=1.5))
saveas(file; width=800, height=350) = save(term="pngcairo size $width,$height fontscale 0.8", output="build/assets/$(file).png")
```


Julia module for working with
[thermobench](https://github.com/CTU-IIG/thermobench)-produced CSV
files.

## Installation

Thermobench.jl can be installed using the Julia package manager. From the Julia REPL, type `]` to enter the Pkg REPL mode and run:

```
pkg> develop /path/to/thermobench/julia
```

Alternatively, if you want to use the same versions of packages as the
author of the package, run:

```
(@v1.4) pkg> activate /path/to/thermobench/julia
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

The simplest way to using the package is the [`multi_fit`](@ref)
function. In the example below, it reads the data from a CSV file and
fits a thermal model to it. The result can be directly plotted by Gnuplot.jl:

```@repl abc
@gp multi_fit("test.csv")
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
mf = multi_fit(["test.csv", "test2.csv"], subtract=:ambient, use_cmpfit=true, use_measurements=true)
@gp mf
saveas("test-mf2") # hide
```
![](assets/test-mf2.png)

In most cases, we are interested only in ``T_∞`` parameters. These can
be plot (and compared between multiple data sets) with [`plot_Tinf`](@ref).

```@repl abc
mf2 = multi_fit(["test.csv", "test2.csv"], :CPU_1_temp, name="CPU1", subtract=:ambient, use_cmpfit=true, use_measurements=true)
@gp T.plot_Tinf(rename!(mf, "CPU0"), mf2) key="left"
saveas("tinf") # hide
```
![](assets/tinf.png)

### Raw thermobench data

To access raw data from thermobench CSV files, use the [`Thermobench.read`](@ref)
function.

```@repl abc
using DataFrames
d = T.read("test.csv");
propertynames(d)
first(d.df, 6)
```

## Reference

```@autodocs
Modules = [Thermobench]
```

```@meta
DocTestSetup = nothing
```
