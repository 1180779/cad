# Ellipse raycasting

## Implicit equation
Ellipse raycasting via solving the ellipse equation in matrix form:
$`f(p) = p^TDp`$
The equation is solved by substituting $`p = o + t\cdot d`$ and solving for $`t`$.
The transformations are applied to the camera rather than the equation, which allows the use of the implicit equation.
Let $`w = Mp`$ be the transformed point. We modify the initial implicit equation by multiplying by identity $`I = MM^{-1} = M^{-1}M`$:
```math
\begin{aligned}
f(p) &= (Ip)^TDIp \\
f(p) &= (M^{-1}Mp)^TDM^{-1}Mp \\
f(p) &= (M^{-1}w)^TDM^{-1}w \\
f(w) &= w^TM^{-T}DM^{-1}w \\
f(w) &= w^TD'w
\end{aligned}
```
where $`D' = M^{-T}DM^{-1}`$. Moreover, if $`M`$ does not contain scaling, then the inverse is computed with a closed-form formula
(check the math library for details).

The rays are generated from the $`[-1, 1]`$ interval on x and y, by using the inverse of the projection view matrix
and constructing rays by selecting $`z = -1`$ and $`z = 1`$ (for OpenGL, NDC is $`[-1, 1]`$ in all dimensions).

## Rendering
The equation is solved on the CPU and the results are buffered to a GPU texture and then displayed.
Rendering uses a user-modifiable adaptation size.

## Multithreading
The CPU buffer filling is parallelized with C++'s std::execution.
