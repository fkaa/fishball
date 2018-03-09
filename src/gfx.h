enum FbGfxShaderType {
    FB_GFX_VERTEX_SHADER,
    FB_GFX_PIXEL_SHADER,
};

struct FbGfxShaderFile {
    const char *path;
    enum FbGfxShaderType type;
};

struct FbGfxShader;

enum FbErrorCode;

enum FbErrorCode GFX_load_shader_files(struct FbGfxShaderFile *files, unsigned int count, struct FbGfxShader **shader);
