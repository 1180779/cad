#version 450 core

#define K_AMBIENT 0.45f
#define K_DIFFUSE 0.55f
#define K_SPECULAR 0.55f
#define ALPHA 5.0f

in vec3 vWorld;
in vec3 vNormal;

uniform bool uFlatShading;
uniform vec3 uCameraPos;
uniform vec3 uColor;

out vec4 FragColor;

void main() {
    const vec3 light = normalize(vec3(0.45f, 1.0f, 0.35f));
    const vec3 normal = uFlatShading
    ? normalize(cross(dFdx(vWorld), dFdy(vWorld)))
    : normalize(vNormal);
    const vec3 toCamera = normalize(uCameraPos - vWorld);
    const vec3 reflected = reflect(-light, normal);

    const float diffuse = max(dot(normal, light), 0.0f);
    const float specular = pow(max(dot(reflected, toCamera), 0.0f), ALPHA);

    FragColor = vec4(
            uColor * (K_AMBIENT + K_DIFFUSE * diffuse + K_SPECULAR * specular),
            1.0f
    );
}
