using Documenter
using Thermobench

DocMeta.setdocmeta!(Thermobench,
                    :DocTestSetup, :(using Thermobench, DataFrames),
                    recursive=true)

using DataFrames

makedocs(
    sitename = "Thermobench",
    format = Documenter.HTML(),
    modules = [Thermobench],
    doctest = true,
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
