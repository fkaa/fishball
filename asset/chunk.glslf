in vec3 ColorVS;
in vec3 NormalVS;

void main()
{
    gl_FragColor = vec4(ColorVS, 1.0);
}
