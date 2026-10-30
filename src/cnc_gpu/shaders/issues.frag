#version 450 core

#define VERTICAL_PLUNGE_SHIFT 0u
#define VERTICAL_PLUNGE_MASK 1u

#define SHANK_COLLISION_MASK 2u
#define SHANK_COLLISION_SHIFT 1u

#define EPS 1e-4

layout(binding = 0) uniform sampler2D uHeights;
layout(std430, binding = 0) buffer IssueResults {
    uint issues[];
};

uniform int uMoveSlot;
uniform vec2 uStockMin;
uniform vec2 uStockCellSize;
uniform float uStockBottom;
uniform float uStockTop;
uniform vec3 uFrom;
uniform vec3 uTo;
uniform float uRadius;
uniform float uCuttingLength;
uniform int uBall;
uniform int uRapid;

void main() {
    ivec2 texel = ivec2(gl_FragCoord.xy);
    float height = texelFetch(uHeights, texel, 0).r;
    if (height <= uStockBottom + EPS) {
        return;
    }

    vec2 point = uStockMin + vec2(texel) * uStockCellSize;
    vec2 delta = uTo.xy - uFrom.xy;
    float lengthSq = dot(delta, delta);
    float radiusSq = uRadius * uRadius;
    uint flags = 0u;

    if (all(equal(uFrom.xy, uTo.xy)) &&
        uTo.z < uFrom.z &&
        uTo.z < uStockTop) {
        vec2 offset = point - uTo.xy;
        float distanceSq = dot(offset, offset);
        float cutterHeight = uTo.z;
        if (uBall != 0) {
            cutterHeight += uRadius - sqrt(max(0.0, radiusSq - distanceSq));
        }
        uint shankCollision = uint(distanceSq <= radiusSq && height > cutterHeight + EPS);
        shankCollision <<= SHANK_COLLISION_SHIFT;
        flags |= shankCollision & SHANK_COLLISION_MASK;
    }

    if (min(uFrom.z, uTo.z) + uCuttingLength < uStockTop) {
        vec2 offset = point - uFrom.xy;
        float enter = 0.0;
        float leave = 1.0;
        bool covered;
        if (lengthSq > EPS) {
            float projection = dot(offset, delta) / lengthSq;
            float distanceSq = max(0.0, dot(offset, offset) - projection * projection * lengthSq);
            float halfInterval = sqrt(max(0.0, radiusSq - distanceSq) / lengthSq);
            enter = max(0.0, projection - halfInterval);
            leave = min(1.0, projection + halfInterval);
            covered = distanceSq < radiusSq &&
                      enter < leave;
        } else {
            covered = dot(offset, offset) < radiusSq;
        }
        float t = (uRapid != 0 && uTo.z < uFrom.z)
            ? leave
            : enter;
        uint verticalPlunge = uint(
                covered &&
                height > mix(uFrom.z, uTo.z, t) + uCuttingLength + EPS
        );
        verticalPlunge <<= VERTICAL_PLUNGE_SHIFT;
        flags |= (verticalPlunge & VERTICAL_PLUNGE_MASK);
    }

    if (flags != 0u) atomicOr(issues[uMoveSlot], flags);
}
