#version 450

layout(location = 0) out vec4 outColor;

void main() {
    vec2 centered = gl_PointCoord - vec2(0.5);
    if (dot(centered, centered) > 0.25) {
        discard;
    }

    outColor = vec4(0.20, 0.75, 1.0, 1.0);
}
