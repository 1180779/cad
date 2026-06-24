#version 450 core

in vec2 uv;
out vec4 FragColor;

uniform sampler2D uLeft;
uniform sampler2D uRight;
uniform int uLuminance; // 0: channel split, 1: luminance

void main()
{
    if (uLuminance == 1) {
        float l = dot(texture(uLeft, uv).rgb, vec3(0.2126, 0.7152, 0.0722));
        float r = dot(texture(uRight, uv).rgb, vec3(0.2126, 0.7152, 0.0722));
        FragColor = vec4(l, r, r, 1.0);
    } else {
        FragColor = vec4(texture(uLeft, uv).r, texture(uRight, uv).gb, 1.0);
    }
}
