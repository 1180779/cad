#version 450 core

uniform ivec2 uStockSize;
uniform vec2 uStockMin;
uniform vec2 uStockCellSize;
uniform float uStockBottom;

uniform vec3 uFrom;
uniform vec3 uTo;
uniform vec2 uDelta;
uniform float uDeltaLengthSq;
uniform float uRadius;

/*
 * 0 = ball
 * 1 = flat
 */
uniform int uType;

out float fragHeight;

void main() {
    const ivec2 texel = ivec2(gl_FragCoord.xy);
    if (texel.x < 0 ||
        texel.y < 0 ||
        texel.x >= uStockSize.x ||
        texel.y >= uStockSize.y) {
        discard;
    }

    const int isBallCutter = int(uType == 0);
    const int isFlatCutter = int(uType == 1);

    const vec3 A = uFrom;
    const vec3 B = uTo;
    const vec2 AtoB = uDelta; // XY plane
    const float AtoBDistSq = uDeltaLengthSq; // XY plane
    const float R = uRadius;
    const float RSq = R * R;
    const vec2 P = uStockMin + vec2(texel) * uStockCellSize;

    int AtoBDistSqGr0 = int(AtoBDistSq > 0.0);
    int AtoBDistSqLe0 = 1 - AtoBDistSqGr0;
    float t0 = dot(P - A.xy, AtoB) / (AtoBDistSq + float(AtoBDistSqLe0));
    float t0c = float(B.z < A.z) * AtoBDistSqLe0
    + clamp(t0, 0.0, 1.0) * AtoBDistSqGr0;

    const vec2 C = A.xy + AtoB * t0c;
    const vec2 CtoP = P - C;
    const float distSq = dot(CtoP, CtoP);
    if (distSq > RSq) {
        discard;
    }

    const float dz = B.z - A.z;
    if (bool(isBallCutter)) {
        if (bool(AtoBDistSqGr0)) {
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
    }
    if (bool(isFlatCutter)) {
        if (bool(AtoBDistSqGr0) && abs(dz) > 1e-3) {
            const float AtoCDist = dot(P - A.xy, AtoB) / AtoBDistSq;
            const vec2 C = A.xy + AtoB * AtoCDist;
            const vec2 CtoP = P - C;
            const float rSq = max(0.0, RSq - dot(CtoP, CtoP));
            const float halfSpan = sqrt(rSq / AtoBDistSq);

            const int dzLt0 = int(dz < 0.0);
            const int dzGe0 = 1 - dzLt0;
            t0c = AtoCDist
                + halfSpan * dzLt0
                - halfSpan * dzGe0;
            t0c = clamp(
                    t0c,
                    0.0,
                    1.0
            );
        }
    }

    const vec2 adjC = A.xy + AtoB * t0c;
    const vec2 adjCtoP = P - adjC;
    const float adjDistSq = dot(adjCtoP, adjCtoP);
    const float zTip = mix(A.z, B.z, t0c);
    const float milledBottom =
        isBallCutter * (zTip + R - sqrt(max(0.0, RSq - adjDistSq)))
        + isFlatCutter * zTip;

    fragHeight = max(milledBottom, uStockBottom);
}
