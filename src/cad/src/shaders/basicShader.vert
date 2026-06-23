#version 450 core

layout (location = 0) in vec3 aPos;

uniform mat4 model;

layout (std140, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
    mat4 VP;
    mat4 invVP;
};

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
