#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aColor;

uniform mat4 model;
uniform vec4 u_overrideColor; // if alpha > 0, overrides per-vertex color

layout (std140, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
    mat4 VP;
    mat4 invVP;
};

out vec4 color;

void main()
{
    color = u_overrideColor.a > 0.0 ? u_overrideColor : aColor;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
