#version 450 core

#define HIGHLIGHT_COLOR vec4(1.0, 0.5, 0.0, 1.0)

in vec4 color;
out vec4 FragColor;

uniform float u_highlightStrength;

void main() {
    FragColor = mix(color, HIGHLIGHT_COLOR, u_highlightStrength);
}
