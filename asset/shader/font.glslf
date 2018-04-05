#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 ColorPS;
layout(location = 1) in vec3 TexCoordPS;

layout(location = 0) out vec4 Color;

layout(binding = 1) uniform sampler2DArray Texture;

void main()
{
    float a = texelFetch(Texture, ivec3(TexCoordPS.xyz), 0).r;

    Color = vec4(ColorPS.rgb, a * ColorPS.a);
}
