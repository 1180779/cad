#version 450 core

layout (isolines, equal_spacing) in;

// number of constant-parameter lines drawn per direction
uniform int uLines;
// slices per iso-line
uniform int uSub;
// 0: line of constant v varying u;
// 1: line of constant u varying v
uniform int uDir;

// which quad of the joined patch this draw call covers, and how many there are
// per direction
uniform int uQuadX;
uniform int uQuadY;
uniform int uCountX;
uniform int uCountY;

in vec3 cpwPointPos[];
flat in int tcInstanceID[];
out vec4 color;
// surface-wide parameters of this point, for trimming in the fragment stage
out vec2 surfaceUv;

layout (std140, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
    mat4 VP;
    mat4 invVP;
};

layout (std140, binding = 1) uniform Palette {
    vec4 lineColor;
    vec4 pointColor;
    vec4 curveColor;
    vec4 gridMinor;
    vec4 gridMajor;
};

vec3 deCasteljau(vec3 p0, vec3 p1, vec3 p2, vec3 p3, float t) {
    vec3 q0 = mix(p0, p1, t);
    vec3 q1 = mix(p1, p2, t);
    vec3 q2 = mix(p2, p3, t);
    vec3 r0 = mix(q0, q1, t);
    vec3 r1 = mix(q1, q2, t);
    return mix(r0, r1, t);
}

// bicubic surface: control[i][j] = cpwPointPos[i*4 + j]; reduce along j (v) then i (u)
vec3 surfacePoint(float u, float v) {
    vec3 row[4];
    for (int i = 0; i < 4; ++i) {
        row[i] = deCasteljau(
                cpwPointPos[i * 4 + 0],
                cpwPointPos[i * 4 + 1],
                cpwPointPos[i * 4 + 2],
                cpwPointPos[i * 4 + 3],
                v
        );
    }
    return deCasteljau(row[0], row[1], row[2], row[3], u);
}

void main() {
    int lineIdx = tcInstanceID[0] / uSub;
    int slice = tcInstanceID[0] % uSub;
    float fixedParam = uLines > 1 ? float(lineIdx) / float(uLines - 1) : 0.0;
    float vary = (float(slice) + gl_TessCoord.x) / float(uSub); // varying

    float localU = uDir == 0 ? vary : fixedParam;
    float localV = uDir == 0 ? fixedParam : vary;
    vec3 pos = surfacePoint(localU, localV);

    surfaceUv = vec2(
            (float(uQuadY) + localU) / float(max(uCountY, 1)),
            (float(uQuadX) + localV) / float(max(uCountX, 1))
    );

    color = curveColor;
    gl_Position = VP * vec4(pos, 1.0);
}
