#version 450

layout(location = 0) in vec3 inUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler s;
layout(set = 0, binding = 2) uniform texture2DArray t;

void main() {
    outColor = vec4(texture(sampler2DArray(t, s), inUV).rgb, 1.0);
}