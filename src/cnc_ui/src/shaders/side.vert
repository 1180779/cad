#version 450 core

layout(location = 0) in vec2 aUV;
layout(location = 1) in float aVertical;
layout(location = 2) in vec3 aNormal;

uniform mat4 uViewProj;
uniform sampler2D uHeights;
uniform ivec2 uSize;
uniform vec3 uMin;
uniform vec3 uWorldSize;

out vec3 vWorld;
out vec3 vNormal;

void main() {
    const ivec2 texel = clamp(
        ivec2(round(aUV * vec2(uSize - 1))),
        ivec2(0),
        uSize - 1
    );
    const float height = texelFetch(uHeights, texel, 0).r;
    const vec2 uv = vec2(texel) / vec2(uSize - 1);

    const vec3 cncPosition = vec3(
        uMin.x + uv.x * uWorldSize.x,
        uMin.y + uv.y * uWorldSize.y,
        mix(uMin.z, height, aVertical)
    );

    // CNC to scene ((X, Y, Z-up) to (X, Y=Z, Z=-Y))
    vWorld = vec3(cncPosition.x, cncPosition.z, -cncPosition.y);
    vNormal = vec3(aNormal.x, aNormal.z, -aNormal.y);
    gl_Position = uViewProj * vec4(vWorld, 1.0);
}
