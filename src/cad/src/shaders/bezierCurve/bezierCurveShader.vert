#version 450 core

layout(location = 0) in vec3 pointPos;
out vec3 wPointPos;
flat out int vInstanceID;

void main() {
    wPointPos = pointPos;
    vInstanceID = gl_InstanceID;
}
