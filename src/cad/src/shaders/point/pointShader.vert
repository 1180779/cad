#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in float aSelected;

layout (std140, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
    mat4 VP;
    mat4 invVP;
};

out float selected;

void main()
{
    selected = aSelected;
    gl_Position = projection * view * vec4(aPos, 1.0);
    gl_PointSize = 8.0;
}
