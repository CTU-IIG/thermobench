using Documenter
using Thermobench

DocMeta.setdocmeta!(Thermobench,
                    :DocTestSetup, :(using Thermobench, DataFrames; cd(joinpath(dirname(pathof(Thermobench)), "..", "test")));
                    recursive=true)
makedocs(
    sitename = "Thermobench",
    format = Documenter.HTML(),
    modules = [Thermobench]
)

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
deploydocs(
    repo = "github.com/CTU-IIG/thermobench.git"
)
