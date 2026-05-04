#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec4 u_overrideColor;// if alpha > 0, overrides per-vertex color

out vec4 color;

void main()
{
    color = u_overrideColor.a > 0.0 ? u_overrideColor : aColor;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
