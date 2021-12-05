#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inUV;

layout(location = 0) out vec3 fragUV;

layout(set = 0, binding = 0) uniform UniformData {
    mat4 proj;
} ubo;

void main() {
    gl_Position = ubo.proj * vec4(inPosition, 1.0);
    fragUV = inUV;
}