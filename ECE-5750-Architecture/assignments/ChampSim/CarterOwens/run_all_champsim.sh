#!/bin/bash

# Script to run ChampSim for all binaries and traces
# Usage: ./run_all_champsim.sh [N_WARM] [N_SIM] [OPTION]
# Default: N_WARM=1, N_SIM=10, OPTION=""

TRACE_DIR=$PWD/dpc3_traces
BIN_DIR=$PWD/bin

# Default parameters
N_WARM=${1:-1}
N_SIM=${2:-10}
OPTION=${3:-}

# Sanity checks
if [ ! -d "$TRACE_DIR" ] ; then
    echo "[ERROR] Cannot find trace directory: $TRACE_DIR"
    exit 1
fi

if [ ! -d "$BIN_DIR" ] ; then
    echo "[ERROR] Cannot find bin directory: $BIN_DIR"
    exit 1
fi

re='^[0-9]+$'
if ! [[ $N_WARM =~ $re ]] || [ -z $N_WARM ] ; then
    echo "[ERROR]: Number of warmup instructions is NOT a number" >&2;
    exit 1
fi

if ! [[ $N_SIM =~ $re ]] || [ -z $N_SIM ] ; then
    echo "[ERROR]: Number of simulation instructions is NOT a number" >&2;
    exit 1
fi

echo "=========================================="
echo "ChampSim Full Benchmark Suite Runner"
echo "=========================================="
echo "Warmup instructions: ${N_WARM}M"
echo "Simulation instructions: ${N_SIM}M"
echo "Option: ${OPTION:-<none>}"
echo "=========================================="
echo ""

# Create results directory
mkdir -p "results_${N_SIM}M"

# Count total jobs
BINARY_COUNT=$(ls -1 "$BIN_DIR" | wc -l)
TRACE_COUNT=$(ls -1 "$TRACE_DIR" | wc -l)
TOTAL_JOBS=$((BINARY_COUNT * TRACE_COUNT))

echo "Found $BINARY_COUNT binaries and $TRACE_COUNT traces"
echo "Total jobs to run: $TOTAL_JOBS"
echo ""

JOB_NUM=0
FAILED_JOBS=()

# Iterate through each binary in bin/
for BINARY in "$BIN_DIR"/*; do
    BINARY_NAME=$(basename "$BINARY")
    
    # Skip if not a regular file
    if [ ! -f "$BINARY" ] || [ ! -x "$BINARY" ]; then
        continue
    fi
    
    # Iterate through each trace in dpc3_traces/
    for TRACE in "$TRACE_DIR"/*; do
        TRACE_NAME=$(basename "$TRACE")
        
        # Skip if not a regular file
        if [ ! -f "$TRACE" ]; then
            continue
        fi
        
        ((JOB_NUM++))
        echo "[$JOB_NUM/$TOTAL_JOBS] Running $BINARY_NAME with $TRACE_NAME..."
        
        # Run the binary
        OUTPUT_FILE="results_${N_SIM}M/${TRACE_NAME}-${BINARY_NAME}${OPTION}.txt"
        ("$BINARY" -warmup_instructions "${N_WARM}000000" -simulation_instructions "${N_SIM}000000" ${OPTION} -traces "${TRACE}") &> "$OUTPUT_FILE"
        
        # Check if the job succeeded
        if [ $? -eq 0 ]; then
            echo "  ✓ Completed: $OUTPUT_FILE"
        else
            echo "  ✗ Failed: $OUTPUT_FILE"
            FAILED_JOBS+=("$BINARY_NAME with $TRACE_NAME")
        fi
    done
done

echo ""
echo "=========================================="
echo "Benchmark Suite Run Complete"
echo "=========================================="
echo "Total jobs run: $JOB_NUM"

if [ ${#FAILED_JOBS[@]} -eq 0 ]; then
    echo "Status: All jobs completed successfully"
else
    echo "Status: ${#FAILED_JOBS[@]} job(s) failed:"
    for job in "${FAILED_JOBS[@]}"; do
        echo "  - $job"
    done
fi

echo "Results saved to: results_${N_SIM}M/"
echo "=========================================="
