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
    if (texel.x < 0 ||
        texel.y < 0 ||
        texel.x >= uStockSize.x ||
        texel.y >= uStockSize.y) {
        discard;
    }
    const vec3 A = uCutterWFrom;
    const vec3 B = uCutterWTo;
    const vec2 AtoB = uCutterDelta; // XY plane
    const float AtoBDistSq = uCutterLengthSquared; // XY plane
    const float R = uCutterRadius;
    const float RSq = R * R;

    const vec2 P = vec2(
            uStockWMinX + float(texel.x) * uStockCellWResolutionX,
            uStockWMinY + float(texel.y) * uStockCellWResolutionY
    );

    float t0c = B.z < A.z ? 1.0 : 0.0;
    float t0;
    if (AtoBDistSq > 0.0) {
        t0 = dot(P - A.xy, AtoB) / AtoBDistSq;
        t0c = clamp(
                t0,
                0.0,
                1.0
        );
    }

    const vec2 C = A.xy + AtoB * t0c;
    const vec2 offset = P - C;
    const float distanceSq = dot(offset, offset);
    if (distanceSq > RSq) {
        discard;
    }

    const float dz = B.z - A.z;
    if (uCutterType == 0) { // ball
        if (AtoBDistSq > 0.0) {
            const vec2 Cu = P - (A.xy + AtoB * t0);
            const float rSq = max(0.0, RSq - dot(Cu, Cu));

            t0c = clamp(
                    t0 - dz * sqrt(
                            rSq / (AtoBDistSq * (AtoBDistSq + dz * dz))
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
    const float zTip = mix(A.z, B.z, t0c);
    float milledBottom = zTip;
    if (uCutterType == 0) {
        milledBottom = zTip + R - sqrt(max(0.0, RSq - adjustedDistanceSq));
    }

    fragHeight = max(milledBottom, uStockWBottom);
}
