#version 450 core

uniform ivec2 uStockSize;
uniform float uStockWMinX;
uniform float uStockWMinY;
uniform float uStockCellWResolutionX;
uniform float uStockCellWResolutionY;
uniform float uStockWBottom;

uniform vec3 uCutterWFrom;
uniform vec3 uCutterWTo;
uniform vec2 uCutterDelta;
uniform float uCutterLengthSquared;
uniform float uCutterRadius;

/*
 * 0 = ball
 * 1 = flat
 */
uniform int uCutterType;

out float fragHeight;

void main() {
    const ivec2 texel = ivec2(gl_FragCoord.xy);
    if (texel.x < 0 || texel.y < 0 || texel.x >= uStockSize.x || texel.y >= uStockSize.y) {
        discard;
    }
    const vec3 A = uCutterWFrom;
    const vec3 B = uCutterWTo;
    const vec2 AtoB = uCutterDelta; // in the XY plane only
    const float AtoBDistSq = uCutterLengthSquared; // in the XY plane only
    const float R = uCutterRadius;
    const float RSq = R * R;

    const vec2 P = vec2(
            uStockWMinX + float(texel.x) * uStockCellWResolutionX,
            uStockWMinY + float(texel.y) * uStockCellWResolutionY
    );

    float t0c = B.z < A.z ? 1.0 : 0.0;
    float t0; // unclamped
    if (AtoBDistSq > 0.0) {
        t0 = dot(P - A.xy, AtoB) / AtoBDistSq;
        t0c = clamp(
                t0,
                0.0,
                1.0
        );
    }

    // C is the closest point to P on the XY move segment.
    const vec2 C = A.xy + AtoB * t0c;
    const vec2 offset = P - C;
    const float distanceSq = dot(offset, offset);
    if (distanceSq > RSq) {
        // cutter doesn't touch current point P at any time
        discard;
    }

    const float dz = B.z - A.z;
    if (uCutterType == 0) {
        if (AtoBDistSq > 0.0) {

            //
            // perpendicular vector from the infinite XY move line to P
            //
            // consider the plane perpendicular to the XY plane and that on which lies the tool path
            // consider then the plane that is parallel to it, but which contains point P
            // let d be the distance between the planes. Lets call them h and g from now on.
            //
            // then let the r be the radius of the intersection of the tool sphere and the plane g
            // we can calculate r from the right triangle C P I
            // where I is a point lying on both plane g and z line through the point P (directly above P)
            //   R^2 = r^2 + d^2
            // where d = |C - P|
            //
            // the smallest z which we are looking for is for the case when line going through
            // P and the intersection circle center is perpendicular to the cutting tool move line
            //
            // in that case we have another right triangle:
            // r^2 = hd^2 + vd^2
            // where hd is the horizontal displacement between intersection circle center and
            // point P:
            //    hd(t)    = AtoBdist   * (t - t0)
            //   (hd(t))^2 = AtoBdistSq * (t - t0)^2
            // and vd^2 is the vertical displacement between intersection circle center and
            // point P:
            //   (vd(t))^2 = (z - A.z + dz*t + R)^2
            // and z(t) is the value we are looking for
            //
            // we have then
            //   r^2 = AtoBdistSq * (t - t0) + (z - A.z + dz*t + R)^2
            //   (z(t) - A.z + dz*t + R)^2 = r^2 - AtoBdistSq * (t - t0)
            // we want the lower z so
            //   z(t) = A.z + dz*t + R - sqrt(r^2 - AtoBdistSq * (t - t0))
            //
            // to find the lowest value we take the derivative and equal to 0:
            //   0 = dz + 1/(sqrt(r^2 - AtoBdistSq * (t - t0)^2)) * AtoBdistSq * (t - t0)
            // let u = t - t0
            //   0 = dz + 1/(sqrt(r^2 - AtoBdistSq * u^2)) * AtoBdistSq * u
            //   -dz * sqrt(r^2 - AtoBdistSq * u^2) = AtoBdistSq * u                    (1) AtoBdistSq > 0
            //   dz^2 * (r^2 - AtoBdistSq * u^2)) = AtoBdistSq^2 * u^2
            //   dz^2 * r^2 - dz^2 * AtoBdistSq * u^2 = AtoBdistSq^2 * u^2
            //   dz^2 * r^2 = u^2 * (AtoBdistSq^2 + dz^2 * AtoBdistSq)
            //   u^2 = dz^2 * r^2 / (AtoBdistSq^2 + dz^2 * AtoBdistSq)
            //
            // we know that sgn(u) = -sgn(dz) so (consider equation before squaring both sides (1))
            //   u = -sqrt( dz^2 * r^2 / (AtoBdistSq^2 + dz^2 * AtoBdistSq) )
            //     u = t - t0 => t = u + t0
            //   t = t0 + sqrt( dz^2 * r^2 / (AtoBdistSq^2 + dz^2 * AtoBdistSq) )
            //   t = t0 + dz * sqrt( r^2 / (AtoBdistSq^2 + dz^2 * AtoBdistSq) )
            //

            const vec2 Cunclamped = P - (A.xy + AtoB * t0);
            const float rSq = max(0.0, RSq - dot(Cunclamped, Cunclamped));

            t0c = clamp(
                    t0 - dz * sqrt(
                            rSq /
                            (AtoBDistSq * (AtoBDistSq + dz * dz))
                    ),
                    0.0,
                    1.0
            );
        }
    } else if (AtoBDistSq > 0.0 && dz != 0.0) {
        const float AtoCDist = dot(P - A.xy, AtoB) / AtoBDistSq;
        const vec2 C = A.xy + AtoB * AtoCDist;
        const vec2 perpendicularToP = P - C;
        const float rSq = max(0.0, RSq - dot(perpendicularToP, perpendicularToP));
        const float halfSpan = sqrt(rSq / AtoBDistSq);
        t0c = clamp(
                dz < 0.0 ? AtoCDist + halfSpan : AtoCDist - halfSpan,
                0.0,
                1.0
        );
    }

    const vec2 adjustedClosest = A.xy + AtoB * t0c;
    const vec2 adjustedOffset = P - adjustedClosest;
    const float adjustedDistanceSq = dot(adjustedOffset, adjustedOffset);

    //
    // for the flat type the result is straightforward
    // for the ball type we need to get intersection z coord
    // for this we solve the intersection of spehrical cutter surface
    //   \[ (x - x_f)^2 + (y - y_f)^2 + (z - z_f)^2 - R_f^2 = 0  \]
    // and vertical line through stock point
    //   \[ x = x_p \wedge y = y_p \wedge z \in \mathbb{R}       \]
    //
    // we get
    //   \[ |z - z_f| =                         \sqrt{ R_f^2 - ((x_f - x_p)^2 + (y_f - y_p)^2) }  \]
    //   \[  z - z_f  =                      +- \sqrt{ R_f^2 - ((x_f - x_p)^2 + (y_f - y_p)^2) }  \]
    //   \[  z        = z_f                  +- \sqrt{ R_f^2 - ((x_f - x_p)^2 + (y_f - y_p)^2) }  \]
    // we also have
    //   \[ z_{\text{tip}}  = z_f - R_f             \]
    //   \[ z_f             = z_{\text{tip}} + R_f  \]
    // so in the end
    //   \[  z        = z_{\text{tip}} + R_f +- \sqrt{ R_f^2 - ((x_f - x_p)^2 + (y_f - y_p)^2) }  \]
    // we take the smaller of the solutions
    //
    const float zTip = mix(A.z, B.z, t0c);
    float milledBottom = zTip;
    if (uCutterType == 0) {
        milledBottom = zTip + R - sqrt(max(0.0, RSq - adjustedDistanceSq));
    }

    fragHeight = max(milledBottom, uStockWBottom);
}
