#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 PositionVS;
layout(location = 1) in vec4 ColorVS;
layout(location = 2) in vec3 TexCoordVS;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out vec4 ColorPS;
layout(location = 1) out vec3 TexCoordPS;

layout(set = 0, binding = 0) uniform Camera
{
    mat4 ModelViewProjection;
};

void main()
{
    ColorPS = ColorVS;
    TexCoordPS = TexCoordVS;
    gl_Position = ModelViewProjection * vec4(PositionVS, 1.0);
}
