#ifndef FB_WND_H
#define FB_WND_H

#include "shared/types.h"
#include <vulkan\vulkan.h>

struct FbWindowConfig {
    char *title;
    unsigned width, height;
    int validation_layer;
};

typedef struct GLFWwindow GLFWwindow;

struct FbWindow {
    GLFWwindow *window_handle;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkImage *swapchain_images;
    VkImageView *swapchain_views;
    VkFramebuffer *swapchain_fbos;
    u32 image_count;
    VkFormat swapchain_format;
    VkExtent2D swapchain_extent;
    VkRenderPass swapchain_renderpass;

    u32 current_image;
    u32 current_buffer;
    u32 buffer_count;
    VkSemaphore *image_semaphores;
    VkSemaphore *finished_semaphores;
};

enum FbErrorCode;
struct FbGpu;
struct GLFWwindow;
typedef struct VkCommandBuffer_T *VkCommandBuffer;

typedef void (* GLFWkeyfun)(struct GLFWwindow*,int,int,int,int);

extern enum FbErrorCode window_new(struct FbWindowConfig cfg, struct FbWindow **wnd, struct FbGpu *gpu);
extern unsigned window_destroy(struct FbWindow *wnd);
extern void     window_set_key_callback(struct FbWindow *wnd, GLFWkeyfun cb);
extern void     window_cxt(struct FbWindow *wnd);
extern void     window_submit(struct FbWindow *wnd, struct FbGpu *gpu, VkCommandBuffer *buffers, u32 buffer_count);
extern void     window_present(struct FbGpu *gpu, struct FbWindow *wnd);
extern u32      window_acquire_image(struct FbGpu *gpu, struct FbWindow *wnd);
extern void     window_swap(struct FbWindow *wnd);
extern int      window_open(struct FbWindow *wnd);
extern void     window_poll(struct FbWindow *wnd);

#endif /* FB_WND_H */
