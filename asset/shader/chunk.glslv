#version 150 core

in vec3 PositionVS;
in vec3 ColorVS;
in vec3 NormalVS;

out vec3 ColorPS;
out vec3 NormalPS;
out vec3 WorldPS;

uniform Camera
{
    mat4 ModelViewProjection;
};

void main()
{
    ColorPS = ColorVS;
    NormalPS = NormalVS;
    WorldPS = PositionVS;
    gl_Position = ModelViewProjection * vec4(PositionVS, 1.0);
}
