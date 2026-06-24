#version 450 core

in vec2 uv;
out vec4 FragColor;

uniform sampler2D uLeft;
uniform sampler2D uRight;

void main()
{
    float r = texture(uLeft, uv).r;
    vec2 gb = texture(uRight, uv).gb;
    FragColor = vec4(r, gb, 1.0);
}
