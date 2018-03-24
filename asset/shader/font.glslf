#version 150 core

in vec4 ColorPS;
in vec3 TexCoordPS;

out vec4 Color;

void main()
{
    Color = ColorPS * TexCoordPS.xyzz;
}
