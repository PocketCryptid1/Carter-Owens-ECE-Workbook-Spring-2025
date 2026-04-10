{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  name = "champsim-dev";
  
  buildInputs = with pkgs; [
    # C++ compiler and build tools
    gcc
    gnumake
    
    # For decompressing traces
    xz
    
    # Useful utilities
    coreutils
    findutils
    gnugrep
    gawk
  ];

  shellHook = ''
    echo "ChampSim Development Environment"
    echo "================================="
    echo "GCC version: $(gcc --version | head -n1)"
    echo "Make version: $(make --version | head -n1)"
    echo ""
    echo "To build ChampSim with BOP prefetcher:"
    echo "  ./build_champsim.sh perceptron no no bop no ship 1"
    echo ""
  '';
}
