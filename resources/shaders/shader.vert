#version 450

layout(location = 0) out vec2 textureCoordinate;

vec2 positions[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(1.0, -1.0),
    vec2(1.0, 1.0),

    vec2(1.0, 1.0),
    vec2(-1.0, 1.0),
    vec2(-1.0, -1.0)
);

vec3 colors[6] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),

    vec3(0.0, 1.0, 1.0),
    vec3(1.0, 0.0, 1.0),
    vec3(1.0, 1.0, 0.0)
);

vec2 textureCoordinates[6] = vec2[](
    vec2(0,0),
    vec2(1,0),
    vec2(1,1),

    vec2(1,1),
    vec2(0,1),
    vec2(0,0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    textureCoordinate = textureCoordinates[gl_VertexIndex];
}