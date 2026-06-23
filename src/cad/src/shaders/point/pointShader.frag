#version 450 core

in float selected;
out vec4 fragColor;

layout (std140, binding = 1) uniform Palette {
    vec4 lineColor;
    vec4 pointColor;
    vec4 curveColor;
    vec4 gridMinor;
    vec4 gridMajor;
};

void main()
{
    // Discard corners to make a round point
    vec2 coord = gl_PointCoord - vec2(0.5);
    if (dot(coord, coord) > 0.25)
    discard;

    vec4 selectedColor = vec4(1.0, 0.5, 0.0, 1.0);
    fragColor = mix(pointColor, selectedColor, selected);
}
