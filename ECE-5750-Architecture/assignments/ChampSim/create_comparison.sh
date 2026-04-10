#!/usr/bin/env bash

echo "=== BOP Implementation vs Reference Implementation Comparison ==="
echo ""
printf "%-27s | %-8s | %-8s | %-8s | %-10s | %-10s\n" "Benchmark" "Our IPC" "Ref IPC" "Diff %" "Our Phases" "Ref Phases"
echo "---------------------------|----------|----------|----------|------------|------------"

benchmarks=(
    "600.perlbench_s-210B"
    "602.gcc_s-734B"
    "603.bwaves_s-3699B"
    "605.mcf_s-665B"
    "607.cactuBSSN_s-2421B"
    "619.lbm_s-4268B"
    "620.omnetpp_s-874B"
    "621.wrf_s-575B"
    "623.xalancbmk_s-700B"
    "625.x264_s-18B"
    "627.cam4_s-573B"
    "628.pop2_s-17B"
    "631.deepsjeng_s-928B"
    "638.imagick_s-10316B"
    "641.leela_s-800B"
    "644.nab_s-5853B"
    "648.exchange2_s-1699B"
    "649.fotonik3d_s-1176B"
    "654.roms_s-842B"
    "657.xz_s-3167B"
)

for bench in "${benchmarks[@]}"; do
    our_file="results_50M/${bench}.champsimtrace.xz-perceptron-no-no-bop-no-ship-1core.txt"
    ref_file="assignment/results_50M/${bench}.champsimtrace.xz-perceptron-no-no-bop-no-ship-1core.txt"
    
    our_ipc=$(grep "CPU 0 cumulative IPC:" "$our_file" 2>/dev/null | head -1 | awk '{print $5}')
    ref_ipc=$(grep "CPU 0 cumulative IPC:" "$ref_file" 2>/dev/null | head -1 | awk '{print $5}')
    
    our_phases=$(grep "bop_total_phases" "$our_file" 2>/dev/null | awk '{print $2}')
    ref_phases=$(grep "bop_total_phases" "$ref_file" 2>/dev/null | awk '{print $2}')
    
    if [[ -n "$our_ipc" && -n "$ref_ipc" ]]; then
        diff=$(awk "BEGIN {printf \"%.2f\", (($our_ipc - $ref_ipc) / $ref_ipc) * 100}")
        printf "%-27s | %-8s | %-8s | %+7s%% | %-10s | %-10s\n" "$bench" "$our_ipc" "$ref_ipc" "$diff" "$our_phases" "$ref_phases"
    fi
done

echo ""
echo "=== Prefetch Accuracy Comparison ==="
echo ""
printf "%-27s | %-12s | %-12s | %-12s | %-12s\n" "Benchmark" "Our Useful" "Our Useless" "Ref Useful" "Ref Useless"
echo "---------------------------|--------------|--------------|--------------|-------------"

for bench in "${benchmarks[@]}"; do
    our_file="results_50M/${bench}.champsimtrace.xz-perceptron-no-no-bop-no-ship-1core.txt"
    ref_file="assignment/results_50M/${bench}.champsimtrace.xz-perceptron-no-no-bop-no-ship-1core.txt"
    
    our_line=$(grep "^L2C PREFETCH  REQUESTED:" "$our_file" 2>/dev/null)
    our_useful=$(echo "$our_line" | awk '{print $8}')
    our_useless=$(echo "$our_line" | awk '{print $10}')
    
    ref_line=$(grep "^L2C PREFETCH  REQUESTED:" "$ref_file" 2>/dev/null)
    ref_useful=$(echo "$ref_line" | awk '{print $8}')
    ref_useless=$(echo "$ref_line" | awk '{print $10}')
    
    printf "%-27s | %-12s | %-12s | %-12s | %-12s\n" "$bench" "$our_useful" "$our_useless" "$ref_useful" "$ref_useless"
done
