#version 450

layout(location = 0) in vec2 textureCoordinate;

layout (binding = 3, rgba8) readonly uniform image2D image;

layout(location = 0) out vec4 outColor;

void main() {
    ivec2 uv = ivec2(textureCoordinate * 256.f);

    outColor = imageLoad(image, uv);
}