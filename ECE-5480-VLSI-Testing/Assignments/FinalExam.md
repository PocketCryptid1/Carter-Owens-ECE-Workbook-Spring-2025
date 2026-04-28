---
title: "ECE 5930 Final Take Home Exam"
geometry: margin=1in
---

## Final Take-Home Exam

---

**Name:** Carter Owens
**A number:** A02399734

---

## Problem 1

### Hardware Diagram

```text
               ----------    ----------    ----------
   fb -------->| D   Q1 |>-->| D   Q2 |>-->| D   Q3 |
               ----------    ----------    ----------
                   |              |              |
                   |              ----[3-NOR]-----
                   |                   |
                   -----[XOR]-----------
                              |
                           [OR]
                              |
                             fb  (feeds back to D of Q1)
```

### Generated Patterns

| Cycle | Q1 | Q2 | Q3 |
|-------|----|----|----|
| 0     |  0 |  0 |  0 |
| 1     |  1 |  0 |  0 |
| 2     |  1 |  1 |  0 |
| 3     |  1 |  1 |  1 |
| 4     |  0 |  1 |  1 |
| 5     |  1 |  0 |  1 |
| 6     |  0 |  1 |  0 |
| 7     |  0 |  0 |  1 |
| 8     |  0 |  0 |  0 |

### Is This Less Hardware Than a 3-Bit Binary Counter?

Yes

## Problem 2


### (a)

Reverse-order steps:

| test | Detected Faults | keep/drop | running set |
| ---- | --------------- | --------- | ----------- |
| t6 | f1, f4, f6, f8 | keep | f1, f4, f6, f8 |
| t5 | f4, f6 | drop | f1, f4, f6, f8 |
| t4 | f1, f2, f7 | keep | f1, f2, f4, f6, f7, f8 |
| t3 | f2, f3, f7 | keep | f1, f2, f3, f4, f6, f7, f8 |
| t2 | f2, f7 | drop | f1, f2, f3, f4, f6, f7, f8 |
| t1 | f3, f5 | keep | f1, f2, f3, f4, f5, f6, f7, f8 |

Reduced set: { t1, t3, t4, t6 }

### (b)

| Fault | t1 | t2 | t3 | t4 | t5 | t6 |
| ----- | -- | -- | -- | -- | -- | -- |
| f1    |    |    |    | X  |    | X  |
| f2    |    | X  | X  | X  |    |    |
| f3    | X  |    | X  |    |    |    |
| f4    |    |    |    |    | X  | X  |
| f5    | X  |    |    |    |    |    |
| f6    |    |    |    |    | X  | X  |
| f7    |    | X  | X  | X  |    |    |
| f8    |    |    |    |    |    | X  |

Minimal set: t1, t3, t6

Proof:

- cant eliminate t1: only test for f5.
- cant eliminate t6: only test for f8.
- at least one test for f2 and f7

## Problem 3

### (a)

```
    C=0
     |
    B=1
   /    \
A=0      A=1
 |      /
null  E=1
       |
      G=0
     /    \
   F=1     F=0
    |           \
   null          G=1
                  |
                 F=0
                /    \
              H=1    H=0
               |      |
              null  Success
```

C=0, B=1, A=1, E=1, G=1, F=0, H=0

### (b)

ABCDEFGH = 110X1010

## Problem 4 (15 points) — Scan Testing

### (a)

For p patterns:

$$T_{cycles} = p \times \left(\frac{n}{m} + 1\right)$$

For the given values:

$$T_{cycles} = 10^6 \times (196 + 1) = 1.97 \times 10^8 \text{ cycles}$$

At 100 MHz:

$$T = 1.97 \times 10^8 \times 10 \text{ ns} = 1{,}970 \text{ ms} \approx 1.97 \text{ s}$$

### (b)

#### Test compression / Illinois Scan

Use on-chip decompressors to expand a small number of external channels into all m scan chains. A 10:1 compression ratio directly gives a 10x reduction in shift cycles with minimal impact on fault coverage.

#### Broadcast scan / identical-chain loading

Load the same pattern into multiple chains simultaneously. Reduces shift cycles but limits the number of unique patterns that can be applied.

#### Partial scan

Only scan a subset of flip-flops, reducing n and therefore scan chain length. but results in lower structural fault coverage for non-scanned flops.

#### At-speed testing with reduced pattern count

Apply fewer but higher-quality patterns, accepting a small fault coverage reduction in exchange for far fewer patterns p.

The best trade-off for high coverage is test compression, it preserves nearly full fault coverage while achieving the required 10x speedup by reducing the effective number of external scan bits shifted per pattern.

## Problem 5 (30 points) — Fault Analysis

### (a)

6

1. f1
2. f2, f6
3. f3, f5
4. f4
5. f7
6. f8

## (b)

t2, t4

## (c)

t1, t2
