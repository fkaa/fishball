#include "window.h"
#include "shared/error.h"

#include "GLFW/glfw3.h"
#include <stdlib.h>
#include <stdio.h>

struct FbWindow {
    GLFWwindow *window_handle;
};

void error_callback(int error, const char* description)
{
    puts(description);
}
unsigned window_new(struct FbWindowConfig cfg, struct FbWindow **wnd)
{
    glfwSetErrorCallback(error_callback);
    glfwInit();

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);

    struct FbWindow window = {0};
    window.window_handle = glfwCreateWindow(cfg.width, cfg.height, cfg.title, NULL, NULL);

    *wnd = malloc(sizeof(**wnd));
    **wnd = window;

    return FB_ERR_NONE;
}

unsigned window_destroy(struct FbWindow *wnd)
{
    glfwDestroyWindow(wnd->window_handle);

    return FB_ERR_NONE;
}

void window_set_key_callback(struct FbWindow *wnd, GLFWkeyfun callback)
{
    glfwSetKeyCallback(wnd->window_handle, callback);
}

void window_cxt(struct FbWindow *wnd)
{
    glfwMakeContextCurrent(wnd->window_handle);
}

void window_swap(struct FbWindow *wnd)
{
    glfwSwapBuffers(wnd->window_handle);
}

int window_open(struct FbWindow *wnd)
{
    return !glfwWindowShouldClose(wnd->window_handle);
}

void window_poll(struct FbWindow *_wnd)
{
    glfwPollEvents();
}

