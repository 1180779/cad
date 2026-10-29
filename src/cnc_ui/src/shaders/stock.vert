#version 450 core

uniform mat4 uViewProj;
uniform sampler2D uHeights;
uniform ivec2 uSize;
uniform vec3 uMin;
uniform vec3 uWorldSize;

out vec3 vWorld;
out vec3 vNormal;

float sampleHeight(const ivec2 texel) {
    return texelFetch(uHeights, clamp(texel, ivec2(0), uSize - 1), 0).r;
}

vec3 heightNormal(const ivec2 texel) {
    const float dx = uWorldSize.x / float(uSize.x - 1);
    const float dy = uWorldSize.y / float(uSize.y - 1);

    const float left = sampleHeight(texel - ivec2(1, 0));
    const float right = sampleHeight(texel + ivec2(1, 0));
    const float down = sampleHeight(texel - ivec2(0, 1));
    const float up = sampleHeight(texel + ivec2(0, 1));

    // CNC to scene ((X, Y, Z-up) to (X, Y=Z, Z=-Y))
    const vec3 normal = normalize(vec3(
            -(right - left) / (2.0 * dx),
            -(up - down) / (2.0 * dy),
            1.0
    ));
    return vec3(normal.x, normal.z, -normal.y);
}

void main() {
    const ivec2 corner = ivec2(
            gl_VertexID & 1,
            (gl_VertexID >> 1) & 1
    );

    const int cellsX = uSize.x - 1;
    const ivec2 cell = ivec2(
            gl_InstanceID % cellsX,
            gl_InstanceID / cellsX
    );
    const ivec2 texel = clamp(
            cell + corner,
            ivec2(0),
            uSize - 1
    );

    const float height = texelFetch(uHeights, texel, 0).r;
    const vec2 uv = vec2(texel) / vec2(uSize - 1);
    const vec3 cncPosition = vec3(
            uMin.x + uv.x * uWorldSize.x,
            uMin.y + uv.y * uWorldSize.y,
            height
    );

    // CNC to scene ((X, Y, Z-up) to (X, Y=Z, Z=-Y))
    vWorld = vec3(cncPosition.x, cncPosition.z, -cncPosition.y);
    vNormal = heightNormal(texel);
    gl_Position = uViewProj * vec4(vWorld, 1.0);
}
