using Documenter
using Thermobench

makedocs(
    sitename = "Thermobench",
    format = Documenter.HTML(),
    modules = [Thermobench]
)

using DocumenterMarkdown
makedocs(
    sitename = "Thermobench",
    format = DocumenterMarkdown.Markdown(),
    build = "build-markdown",
    modules = [Thermobench]
)

# Documenter can also automatically deploy documentation to gh-pages.
# See "Hosting Documentation" and deploydocs() in the Documenter manual
# for more information.
#=deploydocs(
    repo = "<repository url>"
)=#
