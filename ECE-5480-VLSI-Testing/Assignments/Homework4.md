# Homework 4

## Problem 1

### Controllability

| Signal | CC0 | CC1 |
|--------|-----|-----|
| x₁     | 1   | 1   |
| x₂     | 1   | 1   |
| x₃     | 1   | 1   |
| x₄     | 1   | 1   |
| x₅     | 1   | 1   |
| x₆     | 1   | 1   |
| AND(x₂,x₃) output | 2 | 3 |
| AND(x₃,x₄) output | 2 | 3 |
| NOR(x₂x₃, x₃x₄) output | 4 | 3 |
| AND(x₅,x₆) output | 2 | 3 |
| NAND(x₁, NOR_out) output | 5 | 3 |
| AND(NAND_out, x₅x₆) output | 4 | 7 |
| w₁ (final AND) | 5 | 9 |
| w₂ (AND of w₁, x₅x₆) | 5 | 13 |

### Observability

| Signal | CO |
|--------|-----|
| w₁     | 0   |
| w₂     | 0   |
| x₁     | 5   |
| x₂     | 6   |
| x₃     | 5   |
| x₄     | 6   |
| x₅     | 4   |
| x₆     | 4   |

---

## Problem 2

### Controllability

| Signal | CC0 | CC1 |
|--------|-----|-----|
| x₁     | 1   | 1   |
| x₂     | 1   | 1   |
| x₃     | 1   | 1   |
| x₄     | 1   | 1   |
| x₅     | 1   | 1   |
| a (from x₁) | 1 | 1 |
| b (from x₃) | 1 | 1 |
| c (from x₃) | 1 | 1 |
| d (from x₄) | 1 | 1 |
| z₁ (AND x₃,x₄) | 2 | 3 |
| z₂ (NAND x₁,x₂) | 2 | 3 |
| z₃ (NAND x₂,x₃) | 2 | 3 |
| z₄ (AND x₄,x₅) | 2 | 3 |
| e (AND z₂,z₃) | 3 | 5 |
| f (AND z₃,z₄) | 3 | 5 |
| F₁ (AND z₂,e) | 4 | 7 |
| F₂ (AND z₄,f) | 4 | 7 |

### Observability

| Signal | CO |
|--------|-----|
| F₁     | 0   |
| F₂     | 0   |
| x₁     | 5   |
| x₂     | 4   |
| x₃     | 4   |
| x₄     | 4   |
| x₅     | 5   |

---

## Problem 3

### Combinational Controllability

| Signal | CC0 | CC1 |
|--------|-----|-----|
| RESET  | 1   | 1   |
| CLOCK  | 1   | 1   |
| Q₁ (FF1 output) | 1 | 1 |
| Q₂ (FF2 output) | 1 | 1 |
| D₁ (FF1 input, NAND Q₁,Q₂) | 2 | 3 |
| D₂ (FF2 input, from Q₁) | 1 | 1 |
| B(x) (NOR output) | 3 | 2 |
| V(x) (AND B,Q₂) | 3 | 4 |
| D(x) (from Q₂) | 1 | 1 |

### Sequential Controllability

| Signal | SC0 | SC1 |
|--------|-----|-----|
| RESET  | 0   | 0   |
| CLOCK  | 0   | 0   |
| Q₁     | 1   | 1   |
| Q₂     | 1   | 2   |
| D₁     | 0   | 0   |
| D₂     | 0   | 0   |
| B(x)   | 0   | 0   |
| V(x)   | 0   | 0   |
| D(x)   | 0   | 0   |

### Combinational Observability

| Signal | CO |
|--------|-----|
| B(x)   | 0   |
| V(x)   | 0   |
| D(x)   | 0   |
| Q₁     | 2   |
| Q₂     | 2   |
| RESET  | 2   |
| CLOCK  | 2   |

### Sequential Observability

| Signal | SO |
|--------|-----|
| B(x)   | 0   |
| V(x)   | 0   |
| D(x)   | 0   |
| Q₁     | 1   |
| Q₂     | 0   |
| RESET  | 0   |
| CLOCK  | 0   |
