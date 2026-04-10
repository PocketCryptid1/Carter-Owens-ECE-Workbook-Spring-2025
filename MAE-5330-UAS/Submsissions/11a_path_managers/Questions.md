# Chapter 11a Path Managers - Questions

## Question 1
**At the bottom of page 225, it states that the location where the fillet intersects the line is a distance of $\frac{R}{\tan(\rho/2)}$ from the center waypoint. Prove this statement.**

Consider a waypoint $w_i$ with incoming path from $w_{i-1}$ and outgoing path to $w_{i+1}$. Let $\rho$ be the angle between the two path segments at the waypoint (the turn angle).

The fillet circle is tangent to both path lines. Drawing the geometry:

1. Let $d_i$ be the distance from waypoint $w_i$ to where the fillet intersects the line
2. The fillet circle has radius $R$ and its center is perpendicular to each path line at the tangent points
3. The angle $\alpha_i$ between the two path directions satisfies: $\alpha_i = \pi - \rho$, which means the half-angle at the waypoint is $\frac{\alpha_i}{2} = \frac{\pi - \rho}{2}$

Using the right triangle formed by:
- The waypoint $w_i$
- The tangent point on the line
- The fillet circle center

In this right triangle:
- The angle at $w_i$ is $\frac{\rho}{2}$ (half the turn angle)
- The opposite side is $R$ (the radius, perpendicular to the path)
- The adjacent side is $d_i$ (distance along the path to tangent point)

**Applying trigonometry (Law of Sines / basic trig):**

$$\tan\left(\frac{\rho}{2}\right) = \frac{R}{d_i}$$

Solving for $d_i$:

$$\boxed{d_i = \frac{R}{\tan(\rho/2)}}$$

---

## Question 2
**At the bottom of page 225, it states that the center of the circle is $\frac{R}{\sin(\rho/2)}$ from the center waypoint. Prove this statement.**

Using the same right triangle from Question 1, where:
- The angle at the waypoint $w_i$ is $\frac{\rho}{2}$
- The radius $R$ is perpendicular to the path (opposite side)
- The distance from $w_i$ to the fillet center is the hypotenuse, call it $c_i$

**Applying the sine relationship:**

$$\sin\left(\frac{\rho}{2}\right) = \frac{R}{c_i}$$

Solving for $c_i$:

$$\boxed{c_i = \frac{R}{\sin(\rho/2)}}$$

This can also be verified using the Pythagorean theorem with the result from Question 1:
$$c_i = \sqrt{d_i^2 + R^2} = \sqrt{\frac{R^2}{\tan^2(\rho/2)} + R^2} = R\sqrt{\frac{1}{\tan^2(\rho/2)} + 1} = R\sqrt{\frac{\cos^2(\rho/2) + \sin^2(\rho/2)}{\sin^2(\rho/2)}} = \frac{R}{\sin(\rho/2)}$$

---

## Question 3
**There is a singularity in the straight-line switching half-space defined on page 224. When does this singularity occur? Use math to justify your answer.**

The straight-line switching half-space is defined by the plane passing through waypoint $w_i$ with normal vector:

$$\mathbf{n}_i = \frac{\mathbf{q}_{i-1} + \mathbf{q}_i}{||\mathbf{q}_{i-1} + \mathbf{q}_i||}$$

where $\mathbf{q}_{i-1}$ and $\mathbf{q}_i$ are unit vectors along the incoming and outgoing path segments.

**The singularity occurs when:**

$$\mathbf{q}_{i-1} + \mathbf{q}_i = \mathbf{0}$$

This happens when $\mathbf{q}_i = -\mathbf{q}_{i-1}$, meaning the outgoing direction is exactly opposite to the incoming direction.

**Geometrically, this corresponds to:**
- A **180° turn** (i.e., $\rho = \pi$)
- The UAV would need to reverse direction completely
- The waypoints $w_{i-1}$, $w_i$, and $w_{i+1}$ are collinear with $w_i$ between the other two

When $\rho = \pi$, the denominator $||\mathbf{q}_{i-1} + \mathbf{q}_i|| = 0$, causing a **division by zero** singularity.

---

## Question 4
**There are two possibilities for singularities in the definition of the fillet circle center on page 226 as well as a singularity in the halfspace defined on page 227. When do these singularities occur? Describe using math.**

### Singularity 1: Fillet Circle Center Definition

The fillet circle center is located at:

$$\mathbf{c} = w_i - \frac{R}{\sin(\rho/2)} \cdot \frac{\mathbf{q}_{i-1} - \mathbf{q}_i}{||\mathbf{q}_{i-1} - \mathbf{q}_i||}$$

**Singularity occurs when $\rho = 0$:**

When $\rho \to 0$: $\sin(\rho/2) \to 0$

This causes $\frac{R}{\sin(\rho/2)} \to \infty$

**Physical meaning:** When the path is perfectly straight ($\rho = 0$), there is no turn, and the fillet circle would need infinite radius to be tangent to two parallel lines.

### Singularity 2: Unit Vector Normalization

The direction vector $\frac{\mathbf{q}_{i-1} - \mathbf{q}_i}{||\mathbf{q}_{i-1} - \mathbf{q}_i||}$ becomes undefined when:

$$\mathbf{q}_{i-1} = \mathbf{q}_i$$

This also occurs when $\rho = 0$ (straight path), making $||\mathbf{q}_{i-1} - \mathbf{q}_i|| = 0$.

### Singularity 3: Halfspace Normal (Page 227)

Similar to Question 3, the halfspace for fillet transition uses:

$$\mathbf{n}_i = \frac{\mathbf{q}_{i-1} + \mathbf{q}_i}{||\mathbf{q}_{i-1} + \mathbf{q}_i||}$$

**Singularity occurs when $\rho = \pi$:**

When the turn angle is 180°, $\mathbf{q}_i = -\mathbf{q}_{i-1}$, so $\mathbf{q}_{i-1} + \mathbf{q}_i = \mathbf{0}$.

**Summary:**
| Singularity | Condition | Physical Meaning |
|-------------|-----------|------------------|
| $\frac{R}{\sin(\rho/2)}$ | $\rho = 0$ | No turn (straight path) |
| $\|\|\mathbf{q}_{i-1} - \mathbf{q}_i\|\|$ | $\rho = 0$ | No turn (straight path) |
| $\|\|\mathbf{q}_{i-1} + \mathbf{q}_i\|\|$ | $\rho = \pi$ | Complete reversal (180° turn) |

---

## Question 5
**Derive equation (11.4). Note that the law of sines may come in handy.**

Equation (11.4) relates to the geometry of the fillet path. Consider the triangle formed by:
- Waypoint $w_i$  
- The tangent point $z_1$ where the fillet meets the incoming path
- The fillet circle center $c$

Using the **Law of Sines** on this triangle:

$$\frac{a}{\sin A} = \frac{b}{\sin B} = \frac{c}{\sin C}$$

For the fillet geometry:
- The angle at the waypoint is $\frac{\rho}{2}$
- The angle at the tangent point is $90°$ (radius perpendicular to tangent)
- The remaining angle is $90° - \frac{\rho}{2}$

The relevant sides are:
- $R$ = radius (opposite to angle $\frac{\rho}{2}$)
- $d_i$ = distance along path (opposite to $90° - \frac{\rho}{2}$)

Applying Law of Sines:

$$\frac{R}{\sin(\rho/2)} = \frac{d_i}{\sin(90° - \rho/2)} = \frac{d_i}{\cos(\rho/2)}$$

From this:

$$d_i = \frac{R \cos(\rho/2)}{\sin(\rho/2)} = \frac{R}{\tan(\rho/2)}$$

And the distance to the center:

$$c_i = \frac{R}{\sin(\rho/2)}$$

These relationships, combined with the vector definitions for the unit directions $\mathbf{q}_{i-1}$ and $\mathbf{q}_i$, lead to equation (11.4) which defines the transition points and fillet center positions for smooth path following.

$$\boxed{z_i = w_i - d_i \mathbf{q}_{i-1} = w_i - \frac{R}{\tan(\rho/2)} \mathbf{q}_{i-1}}$$
