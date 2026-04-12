#version 450 core

in vec2 fragNDC;

uniform mat4 invVP;
uniform mat4 VP;
uniform mat4 u_model;
uniform vec3 u_axisOrigin;
uniform vec2 u_viewport;
uniform float u_lineWidth;
uniform int u_axesMask;// bit 0=X, bit 1=Y, bit 2=Z

out vec4 FragColor;

#define AXIS_FADE_FAR 1000.0
#define AXIS_FAR  0.9999
#define AXIS_NEAR 0.0001
#define SOFT_EDGE 1.0

const vec3 AXIS_DIR[3] = vec3[3](
vec3(1, 0, 0),
vec3(0, 1, 0),
vec3(0, 0, 1)
);

const vec4 AXIS_COLOR[3] = vec4[3](
vec4(1.0, 0.0, 0.0, 1.0),
vec4(0.0, 1.0, 0.0, 1.0),
vec4(0.0, 0.0, 1.0, 1.0)
);

//
// Returns screen-space pixel distance from this fragment to the closest point on
// the given axis.
//
// nearW; farW - world space near; far points of the ray through the fragment
// d - direction of the axis (passes through u_axisOrigin)
// hitPoint - closest hit point on the axis (world space)
//
// Returns screen-space pixel distance from the closest point on the axis.
//
float axisPixelDist(vec3 nearW, vec3 farW, vec3 d, out vec3 hitPoint)
{
    vec3 r = farW - nearW;

    //
    // https://en.wikipedia.org/wiki/Skew_lines#Nearest_points
    //
    // d <-> d1
    // r <-> d2
    //
    // n is perpendicular to both lines (ray and axis)
    // n = d1 cross d2
    //
    vec3 n = cross(d, r);
    float lenN2 = dot(n, n);
    if (lenN2 < 1e-10)// lines are parallel
    {
        hitPoint = u_axisOrigin;
        return 1e9;
    }

    // n2 = d2 cross n
    // hitPoint = u_axisOrigin + (nearW - u_axisOrigin) dot n2 / (d1 dot n2) * d1
    vec3 n2 = cross(r, n);
    hitPoint = u_axisOrigin + dot(nearW - u_axisOrigin, n2) / (dot(d, n2)) * d;

    vec4 clip = VP * vec4(hitPoint, 1.0);
    if (clip.w <= 0.0) return 1e9;// behind the camera

    // positions in pixels
    vec2 screenHit = ((clip.xy / clip.w) * 0.5 + 0.5) * u_viewport;
    vec2 screenFrag = (fragNDC * 0.5 + 0.5) * u_viewport;
    return length(screenHit - screenFrag);
}

void main()
{
    vec4 nearH = invVP * vec4(fragNDC, -1.0, 1.0);
    vec4 farH = invVP * vec4(fragNDC, 1.0, 1.0);
    vec3 nearW = nearH.xyz / nearH.w;
    vec3 farW = farH.xyz  / farH.w;

    float halfW = u_lineWidth * 0.5;
    float threshold = halfW + SOFT_EDGE;
    float threshold2 = threshold * threshold;

    float bestDist2 = 1e9;
    vec4  bestColor = vec4(0.0);
    float bestDepth = 1.0;

    for (int i = 0; i < 3; i++)
    {
        if ((u_axesMask & (1 << i)) == 0) continue;

        // Transform axis direction by u_model
        vec3 d = normalize((u_model * vec4(AXIS_DIR[i], 0.0)).xyz);

        vec3 hit;
        float pixDist = axisPixelDist(nearW, farW, d, hit);
        float pixDist2 = pixDist * pixDist;
        if (pixDist2 >= threshold2 || pixDist2 >= bestDist2) continue;

        bestDist2 = pixDist2;
        float alpha = 1.0 - smoothstep(halfW - SOFT_EDGE, halfW + SOFT_EDGE, pixDist);
        float distAlongAxis = abs(dot(hit - u_axisOrigin, d));
        alpha *= 1.0 - smoothstep(AXIS_FADE_FAR * 0.5, AXIS_FADE_FAR, distAlongAxis);
        bestColor = vec4(AXIS_COLOR[i].rgb, alpha);

        vec4 clip = VP * vec4(hit, 1.0);
        bestDepth = clamp((clip.z / clip.w) * 0.5 + 0.5, AXIS_NEAR, AXIS_FAR);
    }

    if (bestColor.a < 0.01) discard;
    gl_FragDepth = bestDepth;
    FragColor = bestColor;
}
