module Thermobench

using CSV, LsqFit, DataFrames, Printf, Gnuplot, Colors, Random
using CMPFit
import StatsBase: coef, dof, nobs, rss, stderror, weights, residuals

include("funcs.jl")

export
    @symarray,
    fit,
    interpolate!,
    interpolate,
    plot_fit,
    printfit

end # module
