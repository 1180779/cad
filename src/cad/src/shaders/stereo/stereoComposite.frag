#version 450 core

in vec2 uv;
out vec4 FragColor;

// Left-eye image filtered to red, right-eye image to cyan (green+blue).
// Where both images overlap on a lit object the channels recombine,
// producing the correct blended colour for the intersection.
uniform sampler2D uLeft;
uniform sampler2D uRight;

void main()
{
    float r = texture(uLeft, uv).r;
    vec2 gb = texture(uRight, uv).gb;
    FragColor = vec4(r, gb, 1.0);
}
