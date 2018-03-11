#version 150 core

in vec3 ColorPS;
in vec3 NormalPS;
in vec3 WorldPS;

out vec4 Color;

void main()
{
    Color = vec4(ColorPS, 1.0);
}
