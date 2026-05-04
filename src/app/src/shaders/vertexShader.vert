#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec3 WorldPos;
out vec2 TexCoord;

void main() {
    WorldPos = aPos;
    TexCoord = aTexCoord;
    gl_Position = vec4(WorldPos, 1.0f);
}
