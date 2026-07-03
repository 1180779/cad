#version 450 core

layout (vertices = 16) out;

in vec3 wPointPos[];
flat in int vInstanceID[];
out vec3 cpwPointPos[];
flat out int tcInstanceID[];

layout (std140, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
    mat4 VP;
    mat4 invVP;
};

// viewport size in pixels
uniform vec2 uViewport;
// slices per iso-line
uniform int uSub;

void main() {
    cpwPointPos[gl_InvocationID] = wPointPos[gl_InvocationID];
    tcInstanceID[gl_InvocationID] = vInstanceID[gl_InvocationID];
    if (gl_InvocationID == 0) {
        vec2 mn = vec2(1e30);
        vec2 mx = vec2(-1e30);
        bool behind = false;
        for (int i = 0; i < 16; ++i) {
            vec4 clip = VP * vec4(wPointPos[i], 1.0);
            if (clip.w <= 0.0) {
                behind = true;
                break;
            }
            vec2 px = (clip.xy / clip.w * 0.5 + 0.5) * uViewport;
            mn = min(mn, px);
            mx = max(mx, px);
        }
        float extent = behind ? 4096.0 : max(mx.x - mn.x, mx.y - mn.y);
        gl_TessLevelOuter[0] = 1.0;
        gl_TessLevelOuter[1] = clamp(extent / float(uSub), 4.0, 64.0);
    }
}
