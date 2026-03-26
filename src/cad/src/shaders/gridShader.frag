#version 450 core

#define GRID_COLOR_MINOR vec3(0.72)
#define GRID_COLOR_MAJOR vec3(0.50)

#define GRID_ALPHA_MINOR 0.35
#define GRID_ALPHA_MAJOR 0.80

#define GRID_SCALE_MAJOR 5.0
#define GRID_SCALE_MINOR 1.0

#define GRID_FADE_FAR 300.0
#define GRID_DISCARD_THRESHOLD 0.01
#define GRID_MAX_DEPTH 0.9999
#define GRID_MIN_DEPTH 0.0001

in vec2 fragNDC;

uniform mat4 invVP;
uniform mat4 VP;
// bitmask: bit 0 = XY plane (z=0), bit 1 = XZ plane (y=0), bit 2 = YZ plane (x=0)
uniform int u_gridPlanes;

out vec4 FragColor;

// returns [0, 1] where 1 = on a grid line, 0 = between lines.
float gridAlpha(vec2 coord2D, float scale)
{
    vec2 coord = coord2D / scale;
    vec2 deriv = fwidth(coord);
    vec2 grid  = abs(fract(coord - 0.5) - 0.5) / max(deriv, vec2(0.001));
    return 1.0 - min(min(grid.x, grid.y), 1.0);
}

// Samples one infinite grid plane. axis: 0=XY(z=0), 1=XZ(y=0), 2=YZ(x=0).
// Returns the alpha contribution; writes clamped depth and premultiplied color into out-params.
float samplePlane(vec3 nearW, vec3 farW, int axis, out float outDepth, out vec3 outColorPremult)
{
    outDepth = 1.0;
    outColorPremult = vec3(0.0);

    float nearVal = (axis == 0) ? nearW.z : (axis == 1) ? nearW.y : nearW.x;
    float farVal  = (axis == 0) ? farW.z  : (axis == 1) ? farW.y  : farW.x;
    float dv = nearVal - farVal;
    if (abs(dv) < 1e-6) return 0.0;

    vec3 hit = nearW + (nearVal / dv) * (farW - nearW);

    vec4 clipPos = VP * vec4(hit, 1.0);
    if (clipPos.w <= 0.0) return 0.0;

    float depth = (clipPos.z / clipPos.w) * 0.5 + 0.5;

    vec2 coord2D = (axis == 0) ? hit.xy : (axis == 1) ? hit.xz : hit.yz;
    vec2 inPlaneDelta = (axis == 0) ? (hit.xy - nearW.xy)
    : (axis == 1) ? (hit.xz - nearW.xz)
    :               (hit.yz - nearW.yz);

    float minor = gridAlpha(coord2D, GRID_SCALE_MINOR);
    float major = gridAlpha(coord2D, GRID_SCALE_MAJOR);
    float alpha = max(minor * GRID_ALPHA_MINOR, major * GRID_ALPHA_MAJOR);

    alpha *= 1.0 - smoothstep(GRID_FADE_FAR * 0.5, GRID_FADE_FAR, length(inPlaneDelta));

    if (alpha < GRID_DISCARD_THRESHOLD) return 0.0;

    outDepth = clamp(depth, GRID_MIN_DEPTH, GRID_MAX_DEPTH);
    outColorPremult = mix(GRID_COLOR_MINOR, GRID_COLOR_MAJOR, major) * alpha;
    return alpha;
}

void main()
{
    // reconstruct world-space ray from NDC through near and far planes
    vec4 nearH = invVP * vec4(fragNDC, -1.0, 1.0);
    vec4 farH = invVP * vec4(fragNDC, 1.0, 1.0);
    vec3 nearW = nearH.xyz / nearH.w;
    vec3 farW = farH.xyz  / farH.w;

    // accumulate contributions from each enabled plane
    float totalAlpha = 0.0;
    vec3  totalColorPremult = vec3(0.0);
    float closestDepth = 1.0;
    bool  anyHit = false;

    for (int axis = 0; axis < 3; axis++)
    {
        if ((u_gridPlanes & (1 << axis)) == 0) continue;

        float depth;
        vec3  colorPremult;
        float alpha = samplePlane(nearW, farW, axis, depth, colorPremult);
        if (alpha < GRID_DISCARD_THRESHOLD) continue;

        // front-to-back alpha compositing (order-independent since colors are similar)
        float contrib = alpha * (1.0 - totalAlpha);
        totalColorPremult += colorPremult * (contrib / max(alpha, 0.001));
        totalAlpha += contrib;

        if (!anyHit || depth < closestDepth)
        {
            closestDepth = depth;
            anyHit = true;
        }
    }

    if (!anyHit || totalAlpha < GRID_DISCARD_THRESHOLD) discard;

    gl_FragDepth = closestDepth;
    FragColor = vec4(totalColorPremult, totalAlpha);
}
