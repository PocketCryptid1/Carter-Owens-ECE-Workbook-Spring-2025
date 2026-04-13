{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = [
    pkgs.gcc
    pkgs.clang-tools  # for clangd, clang-format, etc.
    pkgs.gdb
    pkgs.pkg-config
    # Add any libraries you use, e.g. pkgs.opencv, pkgs.boost, etc.
    pkgs.libcamera
  ];
}