#include "shared/error.h"
#include "fbgl.h"
#include "windows.h"
#include "GLFW\glfw3.h"

static void FBGL_load_library(void** lib)
{
    *lib = LoadLibraryA("opengl32.dll");
}

static void FBGL_get_proc(void* lib, void *(**proc)(const char *))
{
    *proc = (void *(*)(const char *))GetProcAddress(lib, "wglGetProcAddress");
}

static void FBGL_close_library(void *lib)
{
    FreeLibrary(lib);
}

void  (*FB_glEnable)();
void  (*FB_glDisable)();

void  (*FB_glGenBuffer)(GLsizei n, GLuint *buffers);
void  (*FB_glDeleteBuffers)(GLsizei n, const GLuint *buffers);
void  (*FB_glBindBuffer)(GLenum target, GLuint buffer);
void *(*FB_glMapBuffer)(GLenum target, GLenum access);

enum FbErrorCode FBGL_load_procs()
{
    void *lib = 0;
    FBGL_load_library(&lib);
    if (!lib) {
        // err
    }

    void *(*FBGL_GetProcAddress)(const char *) = (void *(*)(const char *))glfwGetProcAddress;
    //FBGL_get_proc(lib, &FBGL_GetProcAddress);
    if (!FBGL_GetProcAddress) {
        // err
    }

    FB_glEnable = FBGL_GetProcAddress("glEnable");
    FB_glDisable = FBGL_GetProcAddress("glDisable");

    FB_glGenBuffer = FBGL_GetProcAddress("glGenBuffers");
    FB_glDeleteBuffers = FBGL_GetProcAddress("glDeleteBuffers");
    FB_glBindBuffer = FBGL_GetProcAddress("glBindBuffer");
    FB_glMapBuffer = FBGL_GetProcAddress("glMapBuffer");

    FBGL_close_library(lib);

    return FB_ERR_NONE;
}
