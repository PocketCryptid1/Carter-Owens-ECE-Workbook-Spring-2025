# Best Offset Prefetcher - Experimental Analysis Report

## 1. Introduction

This report presents the experimental analysis of the Best Offset Prefetcher (BOP) implementation for the ECE 5750 Architecture assignment. BOP is a hardware prefetcher that dynamically learns the optimal stride/offset for prefetching based on recent memory access patterns.

## 2. Implementation Overview

### 2.1 Design Summary

The BOP prefetcher consists of three main components:

1. **Recent Request Table (RR)**: A fully-associative 256-entry table that stores recently accessed cache line addresses using FIFO replacement policy.

2. **Offset Score Table**: Maintains scores for 26 candidate offsets during each learning phase.

3. **Learning Phase Logic**: Tests all candidate offsets against RR to find the most effective prefetch distance.

### 2.2 Key Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| MAX_ROUNDS | 100 | Maximum rounds per learning phase |
| RR_SIZE | 256 | Recent Request table entries |
| PREFETCH_DEGREE | 4 | Number of prefetches per trigger |
| SCORE_MAX | 31 | Score threshold for early termination |
| Candidates | 26 | Number of offset candidates |

## 3. Experimental Setup

- **Simulator**: ChampSim
- **Branch Predictor**: Perceptron
- **LLC Replacement**: SHiP
- **Warmup Instructions**: 1 million
- **Simulation Instructions**: 50 million
- **Traces**: 20 SPEC CPU 2017 benchmarks

## 4. Results

### 4.1 IPC Comparison: BOP vs Next-Line Prefetching

| Benchmark | No Prefetch IPC | Next-Line IPC | BOP IPC | BOP Improvement |
|-----------|-----------------|---------------|---------|-----------------|
| 600.perlbench_s | | | | |
| 602.gcc_s | | | | |
| 603.bwaves_s | | | | |
| 605.mcf_s | | | | |
| 607.cactuBSSN_s | | | | |
| 619.lbm_s | | | | |
| 620.omnetpp_s | | | | |
| 621.wrf_s | | | | |
| 623.xalancbmk_s | | | | |
| 625.x264_s | | | | |
| 627.cam4_s | | | | |
| 628.pop2_s | | | | |
| 631.deepsjeng_s | | | | |
| 638.imagick_s | | | | |
| 641.leela_s | | | | |
| 644.nab_s | | | | |
| 648.exchange2_s | | | | |
| 649.fotonik3d_s | | | | |
| 654.roms_s | | | | |
| 657.xz_s | | | | |
| **Geometric Mean** | | | | |

### 4.2 L2C Prefetch Statistics

| Benchmark | Prefetches Issued | L2C Hits | L2C Misses | Useful | Useless |
|-----------|-------------------|----------|------------|--------|---------|
| ... | | | | | |

### 4.3 LLC Statistics

| Benchmark | LLC Accesses | LLC Prefetches | LLC Hits | LLC Misses |
|-----------|--------------|----------------|----------|------------|
| ... | | | | |

## 5. Analysis

### 5.1 Best Performing Benchmarks

[Identify benchmarks where BOP significantly outperforms next-line prefetching]

**Observations:**
- 
- 

**Reasons for Good Performance:**
- 
- 

### 5.2 Worst Performing Benchmarks

[Identify benchmarks where BOP shows minimal improvement or degradation]

**Observations:**
- 
- 

**Reasons for Poor Performance:**
- 
- 

### 5.3 Memory Access Pattern Analysis

[Analyze the memory access patterns of different benchmarks]

#### Streaming Workloads
- Benchmarks with regular stride patterns
- BOP effectiveness

#### Irregular Access Patterns
- Benchmarks with irregular memory access
- BOP challenges

### 5.4 Prefetch Accuracy Analysis

**Useful vs Useless Prefetches:**
- 
- 

**Coverage vs Accuracy Trade-off:**
- 
- 

## 6. Key Insights

### 6.1 Offset Selection Effectiveness

[Discuss how well BOP adapts to different access patterns]

### 6.2 Phase Learning Behavior

[Analyze the learning phase statistics]
- Number of phases completed
- Phases ending due to max rounds vs max score

### 6.3 RR Table Effectiveness

[Discuss the role of RR table in prefetch timing]

## 7. Conclusion

[Summarize the key findings and BOP effectiveness]

## 8. References

1. Michaud, P. (2016). Best-Offset Hardware Prefetching. HPCA 2016.
2. 2nd Data Prefetching Championship (DPC-2)

---

**Note**: This report template should be completed with actual experimental data after running all simulations.
