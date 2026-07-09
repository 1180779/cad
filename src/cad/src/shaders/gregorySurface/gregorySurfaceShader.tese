#version 450 core

layout (isolines, equal_spacing) in;

// number of constant-parameter lines drawn per direction
uniform int uLines;
// slices per iso-line
uniform int uSub;
// 0: line of constant v varying u;
// 1: line of constant u varying v
uniform int uDir;

in vec3 cpwPointPos[];
flat in int tcInstanceID[];
out vec4 color;

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

// rational corner blend
vec3 blendPoint(vec3 fu, float wu, vec3 fv, float wv) {
    return (wu * fu + wv * fv) / max(wu + wv, 1e-6);
}

// Gregory patch (layout per gregory::Net): 
// ring [0..11] runs A(0,0) -> B(1,0) -> C(1,1) -> D(0,1) -> A; 
//[12..19] hold the two interior candidates per corner, collapsed 
// to a bicubic grid by the rational blend
vec3 surfacePoint(float u, float v) {
    vec3 g[4][4]; // g[i][j] = control point at (u = i/3, v = j/3)
    g[0][0] = cpwPointPos[0];
    g[1][0] = cpwPointPos[1];
    g[2][0] = cpwPointPos[2];
    g[3][0] = cpwPointPos[3];
    g[3][1] = cpwPointPos[4];
    g[3][2] = cpwPointPos[5];
    g[3][3] = cpwPointPos[6];
    g[2][3] = cpwPointPos[7];
    g[1][3] = cpwPointPos[8];
    g[0][3] = cpwPointPos[9];
    g[0][2] = cpwPointPos[10];
    g[0][1] = cpwPointPos[11];
    g[1][1] = blendPoint(cpwPointPos[12], u, cpwPointPos[13], v);
    g[2][1] = blendPoint(cpwPointPos[14], 1.0 - u, cpwPointPos[15], v);
    g[2][2] = blendPoint(cpwPointPos[16], 1.0 - u, cpwPointPos[17], 1.0 - v);
    g[1][2] = blendPoint(cpwPointPos[18], u, cpwPointPos[19], 1.0 - v);

    vec3 row[4];
    for (int i = 0; i < 4; ++i) {
        row[i] = deCasteljau(g[i][0], g[i][1], g[i][2], g[i][3], v);
    }
    return deCasteljau(row[0], row[1], row[2], row[3], u);
}

void main() {
    int lineIdx = tcInstanceID[0] / uSub;
    int slice = tcInstanceID[0] % uSub;
    float fixedParam = uLines > 1 ? float(lineIdx) / float(uLines - 1) : 0.0;
    float vary = (float(slice) + gl_TessCoord.x) / float(uSub); // varying

    vec3 pos = uDir == 0
    ? surfacePoint(vary, fixedParam)
    : surfacePoint(fixedParam, vary);

    color = curveColor;
    gl_Position = VP * vec4(pos, 1.0);
}
