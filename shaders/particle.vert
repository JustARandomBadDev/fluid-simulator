#version 450

layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConstants {
    mat4 viewProjection;
} camera;

void main() {
    gl_Position = camera.viewProjection * vec4(inPosition, 1.0);
    gl_PointSize = 5.0;
}
