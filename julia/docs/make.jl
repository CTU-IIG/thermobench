using Documenter
using Thermobench

DocMeta.setdocmeta!(Thermobench,
                    :DocTestSetup, :(using Thermobench, DataFrames),
                    recursive=true)

using DataFrames, Gnuplot
Gnuplot.options.gpviewer = false

# Create gnuplot session now! When it is created later, during
# doctests execution, it results in deadlock. The reason is that
# Gnuplot.jl connects gnuplot's stdout to current Julia's stdout, but
# during doctests, the stdout is redirected to a pipe. The doctest
# waits for EOF on stdout, but this will never happen, because the
# running gnuplot keeps it open indefinitely.
Gnuplot.getsession()

makedocs(
    sitename = "Thermobench",
    format = Documenter.HTML(),
    modules = [Thermobench],
    doctest = true,
    strict = true,
    workdir = @__DIR__,
)
if isinteractive()
    run(`firefox-reload`)
end

# using DocumenterMarkdown
# makedocs(
#     sitename = "Thermobench",
#     format = DocumenterMarkdown.Markdown(),
#     build = "build-markdown",
#     modules = [Thermobench]
# )

# Documenter can also automatically deploy documentation to gh-pages.
# See "Hosting Documentation" and deploydocs() in the Documenter manual
# for more information.
if haskey(ENV, "GITHUB_ACTIONS") && read(`git branch --show-current`, String) == "master\n"
    deploydocs(
        repo = "github.com/CTU-IIG/thermobench.git"
    )
end
