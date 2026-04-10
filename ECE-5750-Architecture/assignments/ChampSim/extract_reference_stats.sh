#!/usr/bin/env bash
# Extract key statistics from reference result files

echo "Benchmark,Ref_IPC,Ref_L2C_Pref_Useful,Ref_L2C_Pref_Useless,Ref_LLC_Miss,Ref_BOP_Phases"

for file in assignment/results_50M/*.txt; do
    # Extract benchmark name
    bench=$(basename "$file" | sed 's/.champsimtrace.xz.*//')
    
    # Extract IPC
    ipc=$(grep "CPU 0 cumulative IPC:" "$file" | head -1 | awk '{print $5}')
    
    # Extract L2C prefetch stats
    l2c_line=$(grep "^L2C PREFETCH  REQUESTED:" "$file")
    l2c_pref_useful=$(echo "$l2c_line" | awk '{print $8}')
    l2c_pref_useless=$(echo "$l2c_line" | awk '{print $10}')
    
    # Extract LLC stats
    llc_miss=$(grep "^LLC TOTAL" "$file" | awk '{print $8}')
    
    # Extract BOP stats
    bop_phases=$(grep "bop_total_phases" "$file" | awk '{print $2}')
    
    echo "$bench,$ipc,$l2c_pref_useful,$l2c_pref_useless,$llc_miss,$bop_phases"
done
