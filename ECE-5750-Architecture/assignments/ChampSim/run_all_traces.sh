#!/usr/bin/env bash
#
# run_all_traces.sh - Script to run BOP prefetcher on all 20 SPEC traces
#
# Usage: ./run_all_traces.sh
#
# This script:
# 1. Builds ChampSim with the BOP prefetcher
# 2. Runs all 20 traces with 1M warmup and 50M simulation instructions
# 3. Saves results to the results_50M directory

# Configuration
WARMUP=1        # 1 million warmup instructions
SIM=50          # 50 million simulation instructions
BINARY="perceptron-no-no-bop-no-ship-1core"

# Traces to run (all 20 SPEC traces)
TRACES=(
    "600.perlbench_s-210B.champsimtrace.xz"
    "602.gcc_s-734B.champsimtrace.xz"
    "603.bwaves_s-3699B.champsimtrace.xz"
    "605.mcf_s-665B.champsimtrace.xz"
    "607.cactuBSSN_s-2421B.champsimtrace.xz"
    "619.lbm_s-4268B.champsimtrace.xz"
    "620.omnetpp_s-874B.champsimtrace.xz"
    "621.wrf_s-575B.champsimtrace.xz"
    "623.xalancbmk_s-700B.champsimtrace.xz"
    "625.x264_s-18B.champsimtrace.xz"
    "627.cam4_s-573B.champsimtrace.xz"
    "628.pop2_s-17B.champsimtrace.xz"
    "631.deepsjeng_s-928B.champsimtrace.xz"
    "638.imagick_s-10316B.champsimtrace.xz"
    "641.leela_s-800B.champsimtrace.xz"
    "644.nab_s-5853B.champsimtrace.xz"
    "648.exchange2_s-1699B.champsimtrace.xz"
    "649.fotonik3d_s-1176B.champsimtrace.xz"
    "654.roms_s-842B.champsimtrace.xz"
    "657.xz_s-3167B.champsimtrace.xz"
)

echo "=============================================="
echo "Best Offset Prefetcher - Running All Traces"
echo "=============================================="

# Step 1: Build ChampSim with BOP prefetcher
echo ""
echo "Building ChampSim with BOP prefetcher..."
echo "Command: ./build_champsim.sh perceptron no no bop no ship 1"
./build_champsim.sh perceptron no no bop no ship 1

if [ ! -f "bin/${BINARY}" ]; then
    echo "ERROR: Build failed! Binary not found: bin/${BINARY}"
    exit 1
fi

echo "Build successful!"

# Step 2: Create results directory
mkdir -p results_${SIM}M

# Step 3: Run all traces
echo ""
echo "Running traces..."
echo "Warmup: ${WARMUP}M instructions"
echo "Simulation: ${SIM}M instructions"
echo ""

for TRACE in "${TRACES[@]}"; do
    echo "Running: ${TRACE}"
    ./run_champsim.sh ${BINARY} ${WARMUP} ${SIM} ${TRACE}
    
    if [ $? -eq 0 ]; then
        echo "  -> Completed successfully"
    else
        echo "  -> ERROR: Failed to run trace"
    fi
done

echo ""
echo "=============================================="
echo "All traces completed!"
echo "Results saved to: results_${SIM}M/"
echo "=============================================="

# List the results
echo ""
echo "Generated result files:"
ls -la results_${SIM}M/*.txt 2>/dev/null || echo "No result files found"
