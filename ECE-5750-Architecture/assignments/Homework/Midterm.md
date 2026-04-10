# 1

## A

|    | Enter ROB | Issue | WB | Commit |   OP   | Dest | Src1 | Src2 |
|----|-----------|-------|----|--------|--------|------|------|------|
| I1 | -1        | 0     | 3  | 4      | fld    | p0   | x1   | -    |
| I2 | 0         | 4     | 9  | 10     | fmul.d | p1   | p0   | f0   |
| I3 | 1         | 1     | 4  | 11     | fld    | p2   | x2   | -    |
| I4 | 2         | 10    | 12 | 13     | fadd.d | p3   | p1   | p0   |
| I5 | 3         | 3     | 6  | 14     | fld    | p4   | x3   | -    |
| I6 | 4         | 13    | 18 | 19     | fmul.d | p5   | p3   | p2   |
| I7 | 5         | 5     | 7  | 20     | fadd.d | p6   | p2   | f4   |

## B

|    | Enter ROB | Issue | WB | Commit |   OP   | Dest | Src1 | Src2 |
|----|-----------|-------|----|--------|--------|------|------|------|
| I1 | -1        | 0     | 3  | 4      | fld    | p0   | x1   | -    |
| I2 | 0         | 4     | 9  | 10     | fmul.d | p1   | p0   | f0   |
| I3 | 1         | 1     | 4  | 11     | fld    | p2   | x2   | -    |
| I4 | 2         | 10    | 12 | 13     | fadd.d | p3   | p1   | p0   |
| I5 | 5         | 5     | 8  | 14     | fld    | p4   | x3   | -    |
| I6 | 11        | 13    | 18 | 19     | fmul.d | p5   | p3   | p2   |
| I7 | 12        | 12    | 14 | 20     | fadd.d | p6   | f2   | f4   |

# 2 

## A

| Arch Reg | Current State | Restored State |
|----------|---------------|----------------|
| x0       | p2            | p2             |
| x1       | p6            | p6             |
| x2       | p3            | p11            |
| x3       | p1            | p1             |
| x4       | p9            | p12            |
| x5       | p5            | p2             |
| x6       | p14           | p4             |
| x7       | p7            | p7             |

## B

When it is no longer the architectural version of of any register, and nothing in-flight needs it

## C

architectural registers + ROB size
8 + 5 = 13

## D

| Instruction       | Destination Register | Freed Register |
|-------------------|----------------------|----------------|
| fld    f0, 0(x3)  | p4                   | p9             |
| fmul.d f0, f0, f1 | p17                  | p4             |
| addi   x3, x3, 8  | p16                  | p13            |
| fsd    f1, 0(x2)  | -                    | -              |
| fld    f1, 0(x4)  | p5                   | p8             |
| fadd.d f1, f2, f1 | p9                   | p5             |
| addi   x4, x4, 8  | p4                   | p21            |
| addi   x2, x2, 8  | p13                  | p6             |

# 3

| Input Addr |                       Main Cache (tag)                       |  Victim Cache (tag)  |
|            |  L0  |  L1  |  L2  |  L3  |  L4  |  L5  |  L6  |  L7  | Hit? | Way 0 | Way 1 | Hit? |
|------------|------|------|------|------|------|------|------|------|------|-------|-------|------|
|  (initial) | inv  | inv  | inv  | inv  | inv  | inv  | inv  | inv  |  -   | inv   | inv   |  -   |
|     00     |  0   |      |      |      |      |      |      |      |  N   |       |       |  N   |
|     80     |  1   |      |      |      |      |      |      |      |  N   |   0   |       |  N   |
|     04     |  0   |      |      |      |      |      |      |      |  N   |   8   |       |  Y   |
|     F4     |      |      |      |      |      |      |      |  1   |  N   |       |       |  N   |
|     20     |      |      |  0   |      |      |      |      |      |  N   |       |       |  N   |
|     C0     |      |      |  1   |      |      |      |      |      |  N   |       |   2   |  N   |
|     78     |      |      |      |      |      |      |      |  0   |  N   |   F   |       |  N   |
|     80     |  1   |      |      |      |      |      |      |      |  N   |       |   0   |  N   |
|     AC     |      |      |      |      |      |      |      |      |  Y   |       |       |  N   |
|     78     |      |      |      |      |      |      |      |      |  Y   |       |       |  N   |
|     FC     |      |      |      |      |      |      |      |  1   |  N   |   7   |       |  Y   |
|     C8     |      |      |      |      |  1   |      |      |      |  N   |       |       |  N   |
|     74     |      |      |      |      |      |      |      |  0   |  N   |   F   |       |  Y   |
|     3C     |      |      |      |  0   |      |      |      |      |  N   |       |       |  N   |
|     A3     |      |      |      |      |      |      |      |      |  Y   |       |       |  N   |
|     8C     |      |      |      |      |      |      |      |      |  Y   |       |       |  N   |
|     B7     |      |      |      |  1   |      |      |      |      |  N   |       |   3   |  N   |