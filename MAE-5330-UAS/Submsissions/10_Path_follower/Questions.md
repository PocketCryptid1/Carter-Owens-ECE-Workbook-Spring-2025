# Path Follower Questions

## Question 1
The equation before (10.8) states that $\dot{W} = -V_a e_{py} \sin(\chi \times \frac{2}{\pi}\tan^{-1}(k_{path}e_{py}))$.

### A. Without skipping the algebra, show how this result is obtained from $W = \frac{1}{2}e_{py}^2$. Assume no wind.

Starting with $W = \frac{1}{2}e_{py}^2$:

$$\dot{W} = e_{py}\dot{e}_{py}$$

From geometry, $\dot{e}_{py} = -V_a \sin(\chi - \chi_q)$ where $\chi_q$ is the path direction.

The control law is $\chi_d = \chi_q - \chi_\infty \frac{2}{\pi}\tan^{-1}(k_{path}e_{py})$.

For tracking analysis, assume $\chi \approx \chi_d$, so:

$$\chi - \chi_q = -\chi_\infty \frac{2}{\pi}\tan^{-1}(k_{path}e_{py})$$

Substituting into $\dot{e}_{py}$:

$$\dot{e}_{py} = -V_a \sin\left(-\chi_\infty \frac{2}{\pi}\tan^{-1}(k_{path}e_{py})\right)$$

$$\dot{e}_{py} = V_a \sin\left(\chi_\infty \frac{2}{\pi}\tan^{-1}(k_{path}e_{py})\right)$$

Therefore:

$$\dot{W} = e_{py}\dot{e}_{py} = V_a e_{py} \sin\left(\chi_\infty \frac{2}{\pi}\tan^{-1}(k_{path}e_{py})\right)$$

With $\chi_\infty$ convention (typically negative for convergence), this becomes:

$$\dot{W} = -V_a e_{py} \sin\left(\chi \times \frac{2}{\pi}\tan^{-1}(k_{path}e_{py})\right)$$

### B. Show that $\dot{W} < 0$ for $e_{py}$.

Consider the behavior by cases:

- When $e_{py} > 0$: The argument $\chi_\infty \frac{2}{\pi}\tan^{-1}(k_{path}e_{py})$ is negative (since $\chi_\infty < 0$), so $\sin(\cdot) < 0$. Thus $\dot{W} = -V_a e_{py} \sin(\cdot) = -V_a (+)(-)  < 0$.

- When $e_{py} < 0$: The argument is positive, so $\sin(\cdot) > 0$. Thus $\dot{W} = -V_a e_{py} \sin(\cdot) = -V_a (-)( +) < 0$.

- When $e_{py} = 0$: $\dot{W} = 0$.

Therefore, $\dot{W} \leq 0$ for all $e_{py}$, proving Lyapunov stability.

## Question 2
In your own words, describe why "$+2\pi m$" is necessary in $\chi_d = atan2(q_e, q_n) + 2\pi m$. Provide two examples to illustrate how this helps the UAV turn in the correct direction.

The $+2\pi m$ term ensures the commanded course is within $\pm\pi$ of the current heading $\chi$, preventing the UAV from turning the "long way around." The `wrap()` function achieves this by adding/subtracting $2\pi$ until $|\chi_d - \chi| < \pi$.

**Example 1:**
- Current heading: $\chi = 3.0$ rad
- Raw path direction: $\text{atan2}(q_e, q_n) = -3.0$ rad
- Without wrap: UAV would turn $6.0$ rad CCW
- With wrap ($m = 1$): $\chi_d = -3.0 + 2\pi = 3.28$ rad, turning only $0.28$ rad CW

**Example 2:**
- Current heading: $\chi = -3.0$ rad  
- Raw path direction: $\text{atan2}(q_e, q_n) = 3.0$ rad
- Without wrap: UAV would turn $6.0$ rad CW
- With wrap ($m = -1$): $\chi_d = 3.0 - 2\pi = -3.28$ rad, turning only $0.28$ rad CCW

## Question 3
Show how $W = \frac{1}{2}(d - \rho)^2$ produces $\dot{W}$ as expressed in the equation located between (10.13) and (10.14).

Starting with $W = \frac{1}{2}(d - \rho)^2$:

$$\dot{W} = (d - \rho)\dot{d}$$

The distance from orbit center is $d = \sqrt{(p_n - c_n)^2 + (p_e - c_e)^2}$.

Taking the derivative:

$$\dot{d} = \frac{1}{2d}\left[2(p_n - c_n)\dot{p}_n + 2(p_e - c_e)\dot{p}_e\right]$$

$$\dot{d} = \frac{(p_n - c_n)\dot{p}_n + (p_e - c_e)\dot{p}_e}{d}$$

With $\dot{p}_n = V_a\cos\chi$ and $\dot{p}_e = V_a\sin\chi$:

$$\dot{d} = \frac{V_a[(p_n - c_n)\cos\chi + (p_e - c_e)\sin\chi]}{d}$$

Let $\varphi = \text{atan2}(p_e - c_e, p_n - c_n)$, then:

$$(p_n - c_n) = d\cos\varphi, \quad (p_e - c_e) = d\sin\varphi$$

$$\dot{d} = V_a[\cos\varphi\cos\chi + \sin\varphi\sin\chi] = V_a\cos(\chi - \varphi)$$

Therefore:

$$\dot{W} = (d - \rho)\dot{d} = V_a(d - \rho)\cos(\chi - \varphi)$$

## Question 4
Show how (10.14) is related to (10.13).

Equation (10.13) gives the orbit course command:

$$\chi_o = \varphi + \lambda\left(\frac{\pi}{2} + \tan^{-1}\left(k_{orbit}\frac{d-\rho}{\rho}\right)\right)$$

Equation (10.14) gives $\dot{W} < 0$ when:

$$\lambda(\chi - \chi_o) \in \left(-\frac{\pi}{2}, \frac{\pi}{2}\right)$$

**Relationship:**

From Question 3, $\dot{W} = V_a(d - \rho)\cos(\chi - \varphi)$. 

Substituting $\chi_o$ from (10.13):

$$\chi - \varphi = \chi - \chi_o + \lambda\left(\frac{\pi}{2} + \tan^{-1}\left(k_{orbit}\frac{d-\rho}{\rho}\right)\right)$$

For $\dot{W} < 0$, need $\cos(\chi - \varphi) \cdot (d - \rho) < 0$.

When the UAV tracks correctly, $\lambda(\chi - \chi_o) \approx 0$, giving:

$$\lambda(\chi - \varphi) \approx \lambda^2\left(\frac{\pi}{2} + \tan^{-1}\left(k_{orbit}\frac{d-\rho}{\rho}\right)\right) = \frac{\pi}{2} + \tan^{-1}\left(k_{orbit}\frac{d-\rho}{\rho}\right)$$

This keeps $\chi - \varphi$ near $\pm\frac{\pi}{2}$ (tangent to circle), ensuring convergence. The condition in (10.14) ensures $\chi$ stays within the stable tracking region around $\chi_o$, maintaining $\dot{W} < 0$.
