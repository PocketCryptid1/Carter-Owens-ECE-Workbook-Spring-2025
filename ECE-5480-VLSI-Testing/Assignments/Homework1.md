# Homework 1

## 1

* G = Good Device, P(G) = 0.8
* B = Bad Device, P(B) = 0.2

P(Test Good | G) = 0.95

P(Test Bad | G ) = 0.05

P(Test Good | B) = 0.05

P(Test Bad  | G) = 0.95

Yield = $$ (P(G) * P(Test Good| G)) + (P(B)*P(Test good | B)) = $$ **77%**

Yield Loss = (P(G) * P(Test Bad | G))/P(G) = **5%**$

Defect Level = (P(B) * P(Test Good | B))/Yield = **1.3%**

## 2

* digital test time: 
  $$ \frac{10^9 * 1024}{500 * 10^6}s = 2048s $$
* Total Test time: 2049.5s
* Annual ATE cost + Depreciation + Maintenance + Operating = $2.369M
* ATE cost per chip (Assuming 24/7 operation): 
  $$ \dfrac{2,369,000}{\frac{31,536,000}{2049.5}} = \$145 \text{ per chip} $$
* adjust for yield: 154/0.7 = **$220**

## 3

(A) 8 nets * 2 = **16**

(B) 2^16 - 1 -16 = **65,519**

(C) 2\*2 + 2\*8 + 6 = **26**

## 4