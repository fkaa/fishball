#version 150 core

in vec4 ColorPS;
in vec3 TexCoordPS;

out vec4 Color;

uniform sampler2DArray Texture;

void main()
{
    float a = texelFetch(Texture, ivec3(TexCoordPS.xyz), 0).r;

    Color = vec4(ColorPS.rgb, a * ColorPS.a);
}
