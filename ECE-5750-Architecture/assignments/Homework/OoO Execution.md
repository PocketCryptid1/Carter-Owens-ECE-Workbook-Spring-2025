## 3.7

Loop:   fld     T9,0(Rx)
I0:     fmul.d  T10,f0,T9
I1:     fdiv.d  T11,f0,T9
I2:     fld     T12,0(Ry)
I3:     fadd.d  T13,f0,T12
I4:     fadd.d  T14,T11,T9
I5:     sd      T12,0(Ry)

## 3.8

| Entry | Initial | After Cycle 1 | After Cycle 2 |
|-------|---------|---------------|---------------|
| f0    | T0      | T0            | T0            |
| f2    | T2      | T2            | T35           |
| f4    | T4      | T4            | T4            |
| f5    | T5      | T32           | T34           |
| f9    | T9      | T33           | T33           |

