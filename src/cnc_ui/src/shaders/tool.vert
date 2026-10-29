#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 uViewProj;
uniform mat4 uModel;

out vec3 vWorld;
out vec3 vNormal;

void main() {
    const mat3 normalMatrix = transpose(inverse(mat3(uModel)));
    vWorld = (uModel * vec4(aPosition, 1.0)).xyz;
    vNormal = normalize(normalMatrix * aNormal);
    gl_Position = uViewProj * vec4(vWorld, 1.0);
}
