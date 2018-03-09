#version 410 core

in vec3 PositionVS;
in vec3 ColorVS;
in vec3 NormalVS;

out vec3 ColorPS;
out vec3 NormalPS;

layout(binding = 0) uniform Camera
{
    mat4 ModelViewProjection;
}

void main()
{
    ColorPS = ColorVS;
    NormalPS = NormalVS;
    gl_Position = vec4(PositionVS, 1.0) * ModelViewProjection;
}