#version 450 core

void main() {
    // fullscreen triangle
    // similarly to ex. here 
    // https://wallisc.github.io/rendering/2021/04/18/Fullscreen-Pass.html
    vec2 position = vec2(
        float((gl_VertexID << 1) & 2),
        float(gl_VertexID & 2)
    );
    gl_Position = vec4(position * 2.0 - 1.0, 0.0, 1.0);
}
