#version 450 core

uniform ivec2 uStockSize;
uniform float uStockWMinX;
uniform float uStockWMinY;
uniform float uStockCellWResolutionX;
uniform float uStockCellWResolutionY;
uniform float uStockWBottom;

uniform vec3 uCutterWFrom;
uniform vec3 uCutterWTo;
uniform float uCutterRadius;

/*
 * 0 = ball 
 * 1 = flat
 */
uniform int uCutterType;

out float fragHeight;

void main() {
    const ivec2 texel = ivec2(gl_FragCoord.xy);
    if (texel.x < 0 || texel.y < 0 || texel.x >= uStockSize.x || texel.y >= uStockSize.y) {
        discard;
    }

    const vec2 position = vec2(
        uStockWMinX + float(texel.x) * uStockCellWResolutionX,
        uStockWMinY + float(texel.y) * uStockCellWResolutionY
    );

    // TODO: move this out the shader to CPU 
    const vec2 delta = uCutterWTo.xy - uCutterWFrom.xy;
    const float lengthSquared = dot(delta, delta);
    //    float t = 0.0;
    //    if (lengthSquared > 1e-10) {
    //        t = clamp(dot(position - uCutterWFrom.xy, delta) / lengthSquared, 0.0, 1.0);
    //    }
    float t = clamp(dot(position - uCutterWFrom.xy, delta) / lengthSquared, 0.0, 1.0);

    const vec2 closest = uCutterWFrom.xy + delta * t;
    const vec2 offset = position - closest;
    const float distanceSq = dot(offset, offset);
    const float cutterRadiusSq = uCutterRadius * uCutterRadius;
    if (distanceSq > cutterRadiusSq) {
        discard;
    }

    // 
    // for the flat type the result is straightforward
    // for the ball type we need to get intersection z coord
    // for this we solve the intersection of sphere cicle cutter
    //   \[ (x - x_f)^2 + (y - y_f)^2 + (z - z_f)^2 - R_f^2 = 0  \]
    // and vertical line through stock point 
    //   \[ x = x_p \wedge y = y_p \wedge z \in \mathbb{R})      \]
    // 
    // we get
    //   \[ |z - z_f| =                         \sqrt{ R_f^2 - ((x_f - x_p)^2 + (y_f - y_p)^2) +- z_f }  \]
    //   \[  z - z_f  =                      +- \sqrt{ R_f^2 - ((x_f - x_p)^2 + (y_f - y_p)^2) +- z_f }  \]
    //   \[  z        = z_f                  +- \sqrt{ R_f^2 - ((x_f - x_p)^2 + (y_f - y_p)^2) +- z_f }  \]
    // we also have 
    //   \[ z_{\text{tip}}  = z_f - R_f             \] 
    //   \[ z_f             = z_{\text{tip}} + R_f  \] 
    // so in the end
    //   \[  z        = z_{\text{tip}} + R_f +- \sqrt{ R_f^2 - ((x_f - x_p)^2 + (y_f - y_p)^2) +- z_f }  \]
    // 
    const float zTip = mix(uCutterWFrom.z, uCutterWTo.z, t);
    float milledBottom = zTip;
    if (uCutterType == 0) {
        milledBottom = zTip + uCutterRadius - sqrt(max(0.0, cutterRadiusSq - distanceSq));
    }

    fragHeight = max(milledBottom, uStockWBottom);
}
