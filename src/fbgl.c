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

void   (*FB_glDebugMessageCallback)(GLDEBUGPROC callback, const void *userParam);
GLuint (*FB_glGetDebugMessageLog)(GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog);

void   (*FB_glEnable)();
void   (*FB_glDisable)();
void   (*FB_glBlendFunc)(GLenum sfactor, GLenum dfactor);
GLubyte *(*FB_glGetString)(GLenum name);

void   (*FB_glGenTextures)(GLsizei n, GLuint *textures);
void   (*FB_glBindTexture)(GLenum target, GLuint texture);
void   (*FB_glActiveTexture)(GLenum texture);
void   (*FB_glTexImage3D)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * data);
void   (*FB_glTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid * pixels);
void   (*FB_glPixelStorei)(GLenum pname, GLint param);
void   (*FB_glTexParameteri)(GLenum target, GLenum pname, GLint param);

void   (*FB_glGenBuffers)(GLsizei n, GLuint *buffers);
void   (*FB_glDeleteBuffers)(GLsizei n, const GLuint *buffers);
void   (*FB_glBindBuffer)(GLenum target, GLuint buffer);
void  *(*FB_glMapBuffer)(GLenum target, GLenum access);
void   (*FB_glUnmapBuffer)(GLenum target);
void  *(*FB_glMapBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
void   (*FB_glFlushMappedBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length);
void   (*FB_glBindBufferRange)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
void   (*FB_glBindBufferBase)(GLenum target, GLuint index, GLuint buffer);
void   (*FB_glBufferData)(GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage);
void   (*FB_glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data);

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

GLint  (*FB_glGetAttribLocation)(GLuint program, const GLchar *name);
GLuint (*FB_glGetUniformBlockIndex)(GLuint program, const GLchar *name);
GLint  (*FB_glGetUniformLocation)(GLuint program, const GLchar *name);
void   (*FB_glGetProgramiv)(GLuint program, GLenum pname, GLint *params);
void   (*FB_glGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
void   (*FB_glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
void   (*FB_glGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
void   (*FB_glUseProgram)(GLuint program);
void   (*FB_glUniform1i)(GLint location, GLint x);

void   (*FB_glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
void   (*FB_glClearColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void   (*FB_glClear)(GLenum targets);

void   (*FB_glDrawArrays)(GLenum primitive, GLint first, GLsizei count);
void   (*FB_glDrawElements)(GLenum primitive, GLsizei count, GLenum type, const GLvoid *indices);

void FBGL_debug_callback(GLenum source,
                         GLenum type,
                         GLuint id,
                         GLenum severity,
                         GLsizei length,
                         const GLchar* message,
                         const void* userParam)
{
    fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}

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
        
    LoadProc(glDebugMessageCallback);
    LoadProc(glGetDebugMessageLog);

    LoadProc(glEnable);
    LoadProc(glDisable);
    LoadProc(glBlendFunc);
    LoadProc(glGetString);

    LoadProc(glGenTextures);
    LoadProc(glBindTexture);
    LoadProc(glActiveTexture);
    LoadProc(glTexImage3D);
    LoadProc(glTexSubImage3D);
    LoadProc(glPixelStorei);
    LoadProc(glTexParameteri);

    LoadProc(glGenBuffers);
    LoadProc(glDeleteBuffers);
    LoadProc(glBindBuffer);
    LoadProc(glMapBuffer);
    LoadProc(glUnmapBuffer);
    LoadProc(glMapBufferRange);
    LoadProc(glFlushMappedBufferRange);
    LoadProc(glBindBufferRange);
    LoadProc(glBindBufferBase);
    LoadProc(glBufferData);
    LoadProc(glBufferSubData);

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

    LoadProc(glGetAttribLocation);
    LoadProc(glGetUniformBlockIndex);
    LoadProc(glGetUniformLocation);
    LoadProc(glGetProgramiv);
    LoadProc(glGetProgramInfoLog);
    LoadProc(glGetShaderiv);
    LoadProc(glGetShaderInfoLog);
    LoadProc(glUseProgram);
    LoadProc(glUniform1i);

    LoadProc(glViewport);
    LoadProc(glClearColor);
    LoadProc(glClear);
    LoadProc(glDrawArrays);
    LoadProc(glDrawElements);

    char *version = glGetString(GL_VERSION);
    char *vendor = glGetString(GL_VENDOR);

    printf("FBGL: %s %s\n", vendor, version);

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback((GLDEBUGPROC) FBGL_debug_callback, 0);

    //FBGL_close_library(lib);

    return FB_ERR_NONE;
}
