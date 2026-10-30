#version 450 core

#define VERTICAL_PLUNGE_SHIFT 1u
#define VERTICAL_PLUNGE_MASK 2u

#define SHANK_COLLISION_MASK 1u
#define SHANK_COLLISION_SHIFT 0u

#define EPS 1e-4

layout(binding = 0) uniform sampler2D uHeights;
layout(std430, binding = 0) buffer IssueResults {
    uint issues[];
};

uniform ivec2 uStockSize;
uniform vec2 uStockMin;
uniform vec2 uStockCellSize;
uniform float uStockBottom;
uniform float uStockTop;

uniform vec3 uFrom;
uniform vec3 uTo;
uniform vec2 uDelta;
uniform float uDeltaLengthSq;
uniform float uRadius;
uniform float uCuttingLength;

/*
 * 0 = ball
 * 1 = flat
 */
uniform int uType;

uniform int uRapid;
uniform int uMoveSlot;

void main() {
    ivec2 texel = ivec2(gl_FragCoord.xy);
    if (texel.x < 0 ||
        texel.y < 0 ||
        texel.x >= uStockSize.x ||
        texel.y >= uStockSize.y) {
        return;
    }
    float height = texelFetch(uHeights, texel, 0).r;
    if (height <= uStockBottom + EPS) {
        return;
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

    uint flags = 0u;

    if (all(equal(A.xy, B.xy)) &&
        B.z < A.z &&
        B.z < uStockTop) {
        vec2 BtoP = P - B.xy;
        float BtoPDistSq = dot(BtoP, BtoP);
        float cutterHeight = isFlatCutter * B.z
            + isBallCutter * (B.z + R - sqrt(max(0.0, RSq - BtoPDistSq)));
        uint verticalPlunge = uint(BtoPDistSq <= RSq && height > cutterHeight + EPS);
        verticalPlunge <<= VERTICAL_PLUNGE_SHIFT;
        flags |= verticalPlunge & VERTICAL_PLUNGE_MASK;
    }

    if (min(A.z, B.z) + uCuttingLength < uStockTop) {
        vec2 AtoP = P - A.xy;
        float enter = 0.0;
        float leave = 1.0;
        bool covered;
        if (AtoBDistSq > EPS) {
            float AtoCDist = dot(AtoP, AtoB) / AtoBDistSq;
            float CtoPDistSq = max(0.0, dot(AtoP, AtoP) - AtoCDist * AtoCDist * AtoBDistSq);
            float halfInterval = sqrt(max(0.0, RSq - CtoPDistSq) / AtoBDistSq);
            enter = max(0.0, AtoCDist - halfInterval);
            leave = min(1.0, AtoCDist + halfInterval);
            covered = CtoPDistSq < RSq &&
                      enter < leave;
        } else {
            covered = dot(AtoP, AtoP) < RSq;
        }
        float t = (uRapid != 0 && B.z < A.z)
            ? leave
            : enter;
        uint shankCollision = uint(
                covered &&
                height > mix(A.z, B.z, t) + uCuttingLength + EPS
        );
        shankCollision <<= SHANK_COLLISION_SHIFT;
        flags |= (shankCollision & SHANK_COLLISION_MASK);
    }

    if (flags != 0u) {
        atomicOr(issues[uMoveSlot], flags);
    }
}
