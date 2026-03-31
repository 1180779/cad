#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in float aSelected;

uniform mat4 view;
uniform mat4 projection;

out float selected;

void main()
{
    selected = aSelected;
    gl_Position = projection * view * vec4(aPos, 1.0);
    gl_PointSize = 8.0;
}
