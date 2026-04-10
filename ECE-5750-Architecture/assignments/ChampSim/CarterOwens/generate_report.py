#!/usr/bin/env python3
"""Parse ChampSim results and generate a report"""

import re
from pathlib import Path
from collections import defaultdict

# Directory with results
results_dir = Path("../results_10M")

# Dictionary to store results: benchmark -> prefetcher -> {ipc, l2_miss_rate}
results = defaultdict(dict)

# Parse all result files
for result_file in sorted(results_dir.glob("*.txt")):
    filename = result_file.name
    
    # Extract benchmark name and prefetcher type
    # Format: XXX.benchmark_name-size.champsimtrace.xz-branch-...-prefetcher-...-1core.txt
    match = re.match(r"([\d\.]+\.\w+_\w-\d+\w)\.champsimtrace\.xz-(\w+)-\w+-\w+(?:-([^\-]+))?", filename)
    
    if not match:
        print(f"Warning: Could not parse filename {filename}")
        continue
    
    benchmark = match.group(1)
    prefetcher = match.group(3) if match.group(3) else "no"
    
    # Read the file and extract metrics
    with open(result_file, 'r') as f:
        content = f.read()
    
    # Extract IPC (cumulative IPC)
    ipc_match = re.search(r"cumulative IPC: ([\d.]+)", content)
    if not ipc_match:
        print(f"Warning: Could not find IPC in {filename}")
        continue
    ipc = float(ipc_match.group(1))
    
    # Extract L2C stats for miss rate calculation
    # L2C TOTAL ACCESS and L2C TOTAL MISS
    l2_total_match = re.search(r"L2C TOTAL\s+ACCESS:\s+(\d+)\s+HIT:\s+\d+\s+MISS:\s+(\d+)", content)
    if not l2_total_match:
        print(f"Warning: Could not find L2C stats in {filename}")
        continue
    
    l2_total_accesses = int(l2_total_match.group(1))
    l2_misses = int(l2_total_match.group(2))
    
    # Calculate L2 miss rate (misses per access)
    if l2_total_accesses > 0:
        l2_miss_rate = l2_misses / l2_total_accesses
    else:
        l2_miss_rate = 0
    
    results[benchmark][prefetcher] = {
        'ipc': ipc,
        'l2_miss_rate': l2_miss_rate,
        'l2_accesses': l2_total_accesses,
        'l2_misses': l2_misses
    }

# Extract benchmark names and sort them
benchmarks = sorted(results.keys(), key=lambda x: int(x.split('.')[0]))

# Prefetcher types (in order of likely appearance)
prefetchers = ['no', 'next_line', 'next_line_advanced', 'next_line_turbo']

# Print table of results for each prefetcher
print("\n" + "="*100)
print("CHAMPSIM RESULTS - PREFETCHER COMPARISON")
print("="*100)

# Table 1: IPC Results
print("\nTable 1: Instructions Per Cycle (IPC) for All Benchmarks")
print("-" * 100)
print(f"{'Benchmark':<25} | {'No Prefetch':<15} | {'Next Line':<15} | {'Next Line Adv':<15} | {'Next Line Turbo':<15}")
print("-" * 100)

for benchmark in benchmarks:
    row_data = [benchmark[:24]]
    for prefetch in prefetchers:
        if prefetch in results[benchmark]:
            ipc = results[benchmark][prefetch]['ipc']
            row_data.append(f"{ipc:.6f}")
        else:
            row_data.append("N/A")
    print(f"{row_data[0]:<25} | {row_data[1]:<15} | {row_data[2]:<15} | {row_data[3]:<15} | {row_data[4]:<15}")

# Table 2: L2 Miss Rate Results
print("\n" + "="*100)
print("\nTable 2: L2 Cache Miss Rate (Misses per Access) for All Benchmarks")
print("-" * 100)
print(f"{'Benchmark':<25} | {'No Prefetch':<15} | {'Next Line':<15} | {'Next Line Adv':<15} | {'Next Line Turbo':<15}")
print("-" * 100)

for benchmark in benchmarks:
    row_data = [benchmark[:24]]
    for prefetch in prefetchers:
        if prefetch in results[benchmark]:
            miss_rate = results[benchmark][prefetch]['l2_miss_rate']
            row_data.append(f"{miss_rate:.6f}")
        else:
            row_data.append("N/A")
    print(f"{row_data[0]:<25} | {row_data[1]:<15} | {row_data[2]:<15} | {row_data[3]:<15} | {row_data[4]:<15}")

# Calculate averages
print("\n" + "="*100)
print("\nAverage Performance Across All Benchmarks:")
print("-" * 100)

for prefetch in prefetchers:
    ipc_values = []
    miss_rate_values = []
    for benchmark in benchmarks:
        if prefetch in results[benchmark]:
            ipc_values.append(results[benchmark][prefetch]['ipc'])
            miss_rate_values.append(results[benchmark][prefetch]['l2_miss_rate'])
    
    if ipc_values:
        avg_ipc = sum(ipc_values) / len(ipc_values)
        avg_miss_rate = sum(miss_rate_values) / len(miss_rate_values)
        print(f"{prefetch:<20}: Avg IPC = {avg_ipc:.6f}, Avg L2 Miss Rate = {avg_miss_rate:.6f}")

print("\n" + "="*100)
