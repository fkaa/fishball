#include "shared/error.h"
#include "fbgl.h"
#include "windows.h"
#include "GLFW\glfw3.h"
#include <stdio.h>

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

void   (*FB_glEnable)();
void   (*FB_glDisable)();
GLubyte *(*FB_glGetString)(GLenum name);

void   (*FB_glGenBuffers)(GLsizei n, GLuint *buffers);
void   (*FB_glDeleteBuffers)(GLsizei n, const GLuint *buffers);
void   (*FB_glBindBuffer)(GLenum target, GLuint buffer);
void  *(*FB_glMapBuffer)(GLenum target, GLenum access);
void   (*FB_glBindBufferRange)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
void   (*FB_glBindBufferBase)(GLenum target, GLuint index, GLuint buffer);
void   (*FB_glBufferData)(GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage);

void   (*FB_glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
void   (*FB_glDisableVertexAttribArray)(GLuint index);
void   (*FB_glEnableVertexAttribArray)(GLuint index);
void   (*FB_glBindVertexArray)(GLuint array);
void   (*FB_glDeleteVertexArrays)(GLsizei n, const GLuint *arrays);
void   (*FB_glGenVertexArrays)(GLsizei n, GLuint *arrays);

void   (*FB_glLinkProgram)(GLuint program);
void   (*FB_glShaderSource)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
void   (*FB_glAttachShader)(GLuint program, GLuint shader);
void   (*FB_glCompileShader)(GLuint shader);
GLuint (*FB_glCreateProgram)(void);
GLuint (*FB_glCreateShader)(GLenum type);
void   (*FB_glDeleteProgram)(GLuint program);
void   (*FB_glDeleteShader)(GLuint shader);

void   (*FB_glGetProgramiv)(GLuint program, GLenum pname, GLint *params);
void   (*FB_glGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
void   (*FB_glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
void   (*FB_glGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
void   (*FB_glUseProgram)(GLuint program);

void   (*FB_glClearColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void   (*FB_glClear)(GLenum targets);

enum FbErrorCode FBGL_load_procs()
{
    void *lib = 0;
    //FBGL_load_library(&lib);
    if (!lib) {
        // err
    }

    void *(*FBGL_GetProcAddress)(const char *) = (void *(*)(const char *))glfwGetProcAddress;
    //FBGL_get_proc(lib, &FBGL_GetProcAddress);
    if (!FBGL_GetProcAddress) {
        // err
    }

#define LoadProc(name) \
    if ((FB_##name = FBGL_GetProcAddress(#name)) == 0) \
        printf("Failed to load FBGL proc: %s\n", #name)
        
    LoadProc(glEnable);
    LoadProc(glDisable);
    LoadProc(glGetString);

    LoadProc(glGenBuffers);
    LoadProc(glDeleteBuffers);
    LoadProc(glBindBuffer);
    LoadProc(glMapBuffer);
    LoadProc(glBindBufferRange);
    LoadProc(glBindBufferBase);
    LoadProc(glBufferData);

    LoadProc(glVertexAttribPointer);
    LoadProc(glDisableVertexAttribArray);
    LoadProc(glEnableVertexAttribArray);
    LoadProc(glBindVertexArray);
    LoadProc(glDeleteVertexArrays);
    LoadProc(glGenVertexArrays);

    LoadProc(glLinkProgram);
    LoadProc(glShaderSource);
    LoadProc(glAttachShader);
    LoadProc(glCompileShader);
    LoadProc(glCreateProgram);
    LoadProc(glCreateShader);
    LoadProc(glDeleteProgram);
    LoadProc(glDeleteShader);

    LoadProc(glGetProgramiv);
    LoadProc(glGetProgramInfoLog);
    LoadProc(glGetShaderiv);
    LoadProc(glGetShaderInfoLog);
    LoadProc(glUseProgram);

    LoadProc(glClearColor);
    LoadProc(glClear);

    char *version = glGetString(GL_VERSION);
    char *vendor = glGetString(GL_VENDOR);

    printf("FBGL: %s %s\n", vendor, version);

    //FBGL_close_library(lib);

    return FB_ERR_NONE;
}
