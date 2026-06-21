#version 450 core

layout (isolines, equal_spacing) in;

uniform int numInstances;
uniform int uLastDegree;// degree of the last segment (1, 2, or 3)
uniform int uLastPrimitive;// index of the last segment (numSegments - 1)

in vec3 cpwPointPos[];
flat in int tcInstanceID[];
out vec4 color;
uniform mat4 MVP;

// lerp until you run out of things to lerp
vec3 deCasteljau(float t, vec3[4] p) {
    if (uLastDegree == 1) {
        return mix(p[0], p[1], t);

    } else if (uLastDegree == 2) {
        vec3 q0 = mix(p[0], p[1], t);
        vec3 q1 = mix(p[1], p[2], t);
        return mix(q0, q1, t);
    } else {
        vec3 q0 = mix(p[0], p[1], t);
        vec3 q1 = mix(p[1], p[2], t);
        vec3 q2 = mix(p[2], p[3], t);
        vec3 r0 = mix(q0, q1, t);
        vec3 r1 = mix(q1, q2, t);
        return mix(r0, r1, t);
    }
}

vec3 deCasteljauCubic(float t, vec3[4] p) {
    vec3 q0 = mix(p[0], p[1], t);
    vec3 q1 = mix(p[1], p[2], t);
    vec3 q2 = mix(p[2], p[3], t);
    vec3 r0 = mix(q0, q1, t);
    vec3 r1 = mix(q1, q2, t);
    return mix(r0, r1, t);
}

void main() {
    float segmentSize = 1.0f / float(numInstances);
    float t = (float(tcInstanceID[0]) + gl_TessCoord.x) * segmentSize;
    vec3 p[4] = vec3[](
            cpwPointPos[0],
            cpwPointPos[1],
            cpwPointPos[2],
            cpwPointPos[3]
    );

    vec3 pos;
    if (gl_PrimitiveID == uLastPrimitive) {
        pos = deCasteljau(t, p);
    } else {
        pos = deCasteljauCubic(t, p);
    }
    color = vec4(0.5, 0.5, 0.5, 1.0);// vec4(0.9, 0.9, 0.9, 1.0);
    gl_Position = MVP * vec4(pos, 1.0f);
}
