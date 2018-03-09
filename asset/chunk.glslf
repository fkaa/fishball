#version 150 core

in vec3 ColorPS;
in vec3 NormalPS;

out vec4 Color;

void main()
{
    Color = vec4(ColorPS, 1.0);
}
