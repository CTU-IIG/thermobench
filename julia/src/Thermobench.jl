module Thermobench

using CSV, LsqFit, DataFrames, Printf, Gnuplot, Colors

include("funcs.jl")

export
    @symarray,
    fit,
    interpolate!,
    interpolate,
    plot_fit,
    printfit

end # module
