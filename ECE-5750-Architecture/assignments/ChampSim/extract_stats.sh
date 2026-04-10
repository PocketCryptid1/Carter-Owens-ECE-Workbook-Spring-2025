#!/usr/bin/env bash
# Extract key statistics from ChampSim result files

echo "Benchmark,IPC,L2C_Pref_Requested,L2C_Pref_Useful,L2C_Pref_Useless,LLC_Total,LLC_Hit,LLC_Miss,BOP_Phases,BOP_MaxRound,BOP_MaxScore,BOP_Pref_Issued"

for file in results_50M/*.txt; do
    # Extract benchmark name
    bench=$(basename "$file" | sed 's/.champsimtrace.xz.*//')
    
    # Extract IPC
    ipc=$(grep "CPU 0 cumulative IPC:" "$file" | head -1 | awk '{print $5}')
    
    # Extract L2C prefetch stats - the line looks like:
    # L2C PREFETCH  REQUESTED:      28961  ISSUED:      28939  USEFUL:        909  USELESS:       3303
    l2c_line=$(grep "^L2C PREFETCH  REQUESTED:" "$file")
    l2c_pref_req=$(echo "$l2c_line" | awk '{print $4}')
    l2c_pref_useful=$(echo "$l2c_line" | awk '{print $8}')
    l2c_pref_useless=$(echo "$l2c_line" | awk '{print $10}')
    
    # Extract LLC stats
    llc_line=$(grep "^LLC TOTAL" "$file")
    llc_total=$(echo "$llc_line" | awk '{print $4}')
    llc_hit=$(echo "$llc_line" | awk '{print $6}')
    llc_miss=$(echo "$llc_line" | awk '{print $8}')
    
    # Extract BOP stats
    bop_phases=$(grep "bop_total_phases" "$file" | awk '{print $2}')
    bop_max_round=$(grep "bop_end_phase_max_round" "$file" | awk '{print $2}')
    bop_max_score=$(grep "bop_end_phase_max_score" "$file" | awk '{print $2}')
    bop_pref_issued=$(grep "bop_pref_issued" "$file" | awk '{print $2}')
    
    echo "$bench,$ipc,$l2c_pref_req,$l2c_pref_useful,$l2c_pref_useless,$llc_total,$llc_hit,$llc_miss,$bop_phases,$bop_max_round,$bop_max_score,$bop_pref_issued"
done
