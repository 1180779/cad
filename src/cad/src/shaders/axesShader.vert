#version 450 core

layout (location = 0) in vec3 aPos;

out vec2 fragNDC;

void main()
{
    fragNDC = aPos.xy;
    gl_Position = vec4(aPos.xy, 1.0, 1.0);
}
