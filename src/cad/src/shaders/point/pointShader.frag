#version 450 core

in float selected;
out vec4 fragColor;

void main()
{
    // Discard corners to make a round point
    vec2 coord = gl_PointCoord - vec2(0.5);
    if (dot(coord, coord) > 0.25)
    discard;

    vec4 normalColor   = vec4(0.08, 0.08, 0.08, 1.0);
    vec4 selectedColor = vec4(1.0, 0.5, 0.0, 1.0);
    fragColor = mix(normalColor, selectedColor, selected);
}
