#version 450 core

layout (vertices = 16) out;

in vec3 wPointPos[];
flat in int vInstanceID[];
out vec3 cpwPointPos[];
flat out int tcInstanceID[];

void main() {
    cpwPointPos[gl_InvocationID] = wPointPos[gl_InvocationID];
    tcInstanceID[gl_InvocationID] = vInstanceID[gl_InvocationID];
    if (gl_InvocationID == 0) {
        gl_TessLevelOuter[0] = 1.0;
        gl_TessLevelOuter[1] = 64.0;
    }
}
