#version 450 core

layout (isolines, equal_spacing) in;

uniform int numInstances;
uniform int uLastDegree;// degree of the last segment (1, 2, or 3)
uniform int uLastPrimitive;// index of the last segment (numSegments - 1)

in vec3 cpwPointPos[];
flat in int tcInstanceID[];
out vec4 color;
uniform mat4 MVP;

vec3 deCasteljau(float t) {
    vec3 p[4] = vec3[](
    cpwPointPos[0],
    cpwPointPos[1],
    cpwPointPos[2],
    cpwPointPos[3]
    );

    if (uLastDegree == 1) {
        // Linear
        return mix(p[0], p[1], t);

    } else if (uLastDegree == 2) {
        // Quadratic
        vec3 q0 = mix(p[0], p[1], t);
        vec3 q1 = mix(p[1], p[2], t);
        return mix(q0, q1, t);

    } else {
        // Cubic (degree 3)
        vec3 q0 = mix(p[0], p[1], t);
        vec3 q1 = mix(p[1], p[2], t);
        vec3 q2 = mix(p[2], p[3], t);
        vec3 r0 = mix(q0, q1, t);
        vec3 r1 = mix(q1, q2, t);
        return mix(r0, r1, t);
    }
}

vec3 deCasteljauCubic(float t) {
    float u = 1.0f - t;
    float b0 = u * u * u;
    float b1 = 3.0 * u * u * t;
    float b2 = 3.0 * u * t * t;
    float b3 = t * t * t;
    return b0 * cpwPointPos[0] + b1 * cpwPointPos[1] + b2 * cpwPointPos[2] + b3 * cpwPointPos[3];
}

void main() {
    float segmentSize = 1.0f / float(numInstances);
    float t = (float(tcInstanceID[0]) + gl_TessCoord.x) * segmentSize;

    vec3 pos;
    if (gl_PrimitiveID == uLastPrimitive) {
        pos = deCasteljau(t);
    } else {
        pos = deCasteljauCubic(t);
    }
    color = vec4(0.5, 0.5, 0.5, 1.0);// vec4(0.9, 0.9, 0.9, 1.0);
    gl_Position = MVP * vec4(pos, 1.0f);
}
