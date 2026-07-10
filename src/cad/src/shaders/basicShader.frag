#version 450 core

out vec4 FragColor;

layout (std140, binding = 1) uniform Palette {
    vec4 lineColor;
    vec4 pointColor;
    vec4 curveColor;
    vec4 gridMinor;
    vec4 gridMajor;
};

void main() {
    FragColor = lineColor;
}
