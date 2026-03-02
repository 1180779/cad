#version 430 core

uniform vec3 color = vec3(1.0, 0.0, 1.0);
uniform float alpha = 1.0;

out vec4 FragColor;

void main()
{
    FragColor = vec4(color, alpha);
}



