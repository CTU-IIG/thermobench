# To cross-compile build this with:
#     nix build --arg crossSystem '{ config = "aarch64-unknown-linux-gnu"; }'
{
  crossSystem ? null,
  nixpkgs ? <nixpkgs>,
  pkgs ? import nixpkgs { crossSystem = crossSystem; }
}:
with pkgs;
stdenv.mkDerivation {
  name = "thermobech";
  # src = ./.;
  src = builtins.fetchGit {
    url = ./.;
    ## Currently released Nix does not support submodule fetching, so
    ## until next release, we live without the benchmarks.
    # submodules = true;
  };
  nativeBuildInputs = [ meson ninja ];
  buildInputs = [ libev ];
}
