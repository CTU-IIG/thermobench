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
  mesonFlags = "-Dauto_features=enabled";

  # Meson is no longer able to pick up Boost automatically.
  # https://github.com/NixOS/nixpkgs/issues/86131
  BOOST_INCLUDEDIR = "${stdenv.lib.getDev boost}/include";
  BOOST_LIBRARYDIR = "${stdenv.lib.getLib boost}/lib";

  nativeBuildInputs = [ meson ninja pkg-config ];
  buildInputs = [ libev opencl-headers ocl-icd boost ];
}
