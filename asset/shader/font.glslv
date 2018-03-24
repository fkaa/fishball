#version 150 core

in vec3 PositionVS;
in vec4 ColorVS;
in vec3 TexCoordVS;

out vec4 ColorPS;
out vec3 TexCoordPS;

uniform Camera
{
    mat4 ModelViewProjection;
};

void main()
{
    ColorPS = ColorVS;
    TexCoordPS = TexCoordVS;
    gl_Position = ModelViewProjection * vec4(PositionVS, 1.0);
}
