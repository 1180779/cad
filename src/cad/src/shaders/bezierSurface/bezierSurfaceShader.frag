#version 450 core

#define HIGHLIGHT_COLOR vec4(1.0, 0.5, 0.0, 1.0)

in vec4 color;
in vec2 surfaceUv;
out vec4 FragColor;

uniform float u_highlightStrength;

uniform sampler2D uTrimMask;
uniform bool uTrimEnabled;

void main() {
    if (uTrimEnabled && texture(uTrimMask, surfaceUv).r < 0.5) {
        discard;
    }
    FragColor = mix(color, HIGHLIGHT_COLOR, u_highlightStrength);
}
