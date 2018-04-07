#include "window.h"
#include "shared/error.h"
#include "shared/types.h"
#include "array.h"
#include "gfx.h"

#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void error_callback(int error, const char* description)
{
    puts(description);
}

VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
    PFN_vkCreateDebugReportCallbackEXT func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != 0) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL WND_VK_callback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layerPrefix,
    const char* msg,
    void* userData)
{
    printf("%s\n", msg);

    return VK_FALSE;
}

bool WND_supports_validation_layers(const char **layers, u32 layer_count)
{
    u32 supported_layer_count = 0;
    vkEnumerateInstanceLayerProperties(&supported_layer_count, 0);

    VkLayerProperties *supported_layers = malloc(supported_layer_count * sizeof(*supported_layers));
    vkEnumerateInstanceLayerProperties(&supported_layer_count, supported_layers);

    for (u32 i = 0; i < layer_count; ++i) {
        bool found = false;

        for (u32 j = 0; j < supported_layer_count; ++j) {
            if (strcmp(layers[i], supported_layers[j].layerName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            free(supported_layers);
            return false;
        }
    }

    free(supported_layers);
    return true;
}

void WND_get_surface_format(VkPhysicalDevice device, VkSurfaceKHR surface, VkSurfaceFormatKHR *desired_formats, u32 desired_len, VkSurfaceFormatKHR *out_format)
{
    u32 count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, 0);
    if (count != 0) {
        VkSurfaceFormatKHR *formats = malloc(count * sizeof(*formats));
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, formats);

        if (desired_len > 0 && desired_formats) {
            for (u32 i = 0; i < desired_len; ++i) {
                VkSurfaceFormatKHR desired = desired_formats[i];
                for (u32 j = 0; j < count; ++j) {
                    VkSurfaceFormatKHR format = formats[j];
                    if (desired.format == format.format && desired.colorSpace == format.colorSpace) {
                        *out_format = format;
                        free(formats);
                        return;
                    }
                }
            }
        }
    }
}

bool WND_get_present_mode(VkPhysicalDevice device, VkSurfaceKHR surface, VkPresentModeKHR *desired_modes, u32 desired_len, VkPresentModeKHR *out_mode)
{
    u32 count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, 0);
    if (count != 0) {
        VkPresentModeKHR *modes = malloc(count * sizeof(*modes));
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, modes);

        if (desired_len > 0 && desired_modes) {
            for (u32 i = 0; i < desired_len; ++i) {
                VkPresentModeKHR desired = desired_modes[i];
                for (u32 j = 0; j < count; ++j) {
                    VkPresentModeKHR mode = modes[j];
                    if (desired == mode) {
                        *out_mode = mode;
                        free(modes);
                        return true;
                    }
                }
            }
        }

        *out_mode = *modes;
        free(modes);
        return true;
    }

    return false;
}

VkExtent2D WND_get_swapchain_extents(VkSurfaceCapabilitiesKHR *capabilities, u32 width, u32 height) {
    if (capabilities->currentExtent.width != 0xffffffff) {
        return capabilities->currentExtent;
    } else {
        VkExtent2D actual = { width, height };

        actual.width = max(capabilities->minImageExtent.width, min(capabilities->maxImageExtent.width, actual.width));
        actual.height = max(capabilities->minImageExtent.height, min(capabilities->maxImageExtent.height, actual.height));

        return actual;
    }
}

enum FbErrorCode window_new(struct FbWindowConfig cfg, struct FbWindow **wnd, struct FbGpu *gpu)
{
    glfwSetErrorCallback(error_callback);
    glfwInit();

    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "fishball";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "f8";
    app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo = &app_info;

    u32 extension_count = 0;
    vkEnumerateInstanceExtensionProperties(0, &extension_count, 0);
    VkExtensionProperties *available_extensions = malloc(extension_count * sizeof(*available_extensions));
    vkEnumerateInstanceExtensionProperties(0, &extension_count, available_extensions);
    printf("%d available extension(s):\n", extension_count);
    for (u32 i = 0; i < extension_count; ++i) {
        printf("  #%d: %s\n", i, available_extensions[i].extensionName);
    }

    u32 required_extension_count = 0;
    const char **required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);
    const char **extensions = 0;

    for (u32 i = 0; i < required_extension_count; ++i) {
        ARRAY_push(extensions, required_extensions[i]);
    }

    const char *layers[1] = {
        "VK_LAYER_LUNARG_standard_validation"
    };

    if (cfg.validation_layer) {
        if (!WND_supports_validation_layers(layers, 1)) {
            return ERR_fmt(FB_ERR_VK_VALIDATION_LAYER, "driver doesn't support validation layer");
        }

        info.enabledLayerCount = 1;
        info.ppEnabledLayerNames = layers;

        ARRAY_push(extensions, VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    } else {
        info.enabledLayerCount = 0;
    }


    info.enabledExtensionCount = ARRAY_size(extensions);
    info.ppEnabledExtensionNames = extensions;

    VkInstance instance;
    VkResult result = vkCreateInstance(&info, 0, &instance);

    if (cfg.validation_layer) {
        VkDebugReportCallbackEXT callback = {0};
        VkDebugReportCallbackCreateInfoEXT callback_info = {0};
        callback_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        callback_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        callback_info.pfnCallback = WND_VK_callback;

        if (CreateDebugReportCallbackEXT(instance, &callback_info, 0, &callback) != VK_SUCCESS) {
            return ERR_fmt(FB_ERR_VK_DEBUG_CALLBACK, "failed to register debug report callback");
        }
    }

    VkPhysicalDevice physical_device = {0};
    u32 device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, 0);
    if (device_count == 0) {
        return ERR_fmt(FB_ERR_VK_NO_DEVICE, "could not find any physical device");
    }
    VkPhysicalDevice *devices = malloc(device_count * sizeof(*devices));
    vkEnumeratePhysicalDevices(instance, &device_count, devices);

    printf("%d physical device(s):\n", device_count);
    for (u32 i = 0; i < device_count; ++i) {
        VkPhysicalDeviceProperties device_prop = {0};
        VkPhysicalDeviceFeatures device_features = {0};

        vkGetPhysicalDeviceProperties(devices[i], &device_prop);
        vkGetPhysicalDeviceFeatures(devices[i], &device_features);

        printf("  #%d: %s\n", i, device_prop.deviceName);

        physical_device = devices[i];
        break;
    }
    free(devices);

    printf("%d extension(s):\n", ARRAY_size(extensions));
    for (u32 i = 0; i < ARRAY_size(extensions); ++i) {
        printf("  #%d: %s\n", i, extensions[i]);
    }

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, 0);
    VkQueueFamilyProperties *queue_family_properties = malloc(queue_family_count * sizeof(*queue_family_properties));
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties);
    printf("%d queue(s):\n", queue_family_count);
    for (u32 i = 0; i < queue_family_count; ++i) {
        VkQueueFlagBits queue_flags = queue_family_properties[i].queueFlags;
        printf("  #%d: %c%c%c%c (%d)\n", i,
            (queue_flags & VK_QUEUE_GRAPHICS_BIT) ? 'G' : '_',
            (queue_flags & VK_QUEUE_COMPUTE_BIT) ? 'C' : '_',
            (queue_flags & VK_QUEUE_TRANSFER_BIT) ? 'T' : '_',
            (queue_flags & VK_QUEUE_SPARSE_BINDING_BIT) ? 'S' : '_',
            queue_family_properties[i].queueCount
        );
    }
    free(queue_family_properties);


    VkPhysicalDeviceFeatures device_features = {0};

    VkDeviceQueueCreateInfo queue_create_info = {0};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = 0;
    queue_create_info.queueCount = 1;
    r32 priority = 1.f;
    queue_create_info.pQueuePriorities = &priority;

    VkDeviceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &queue_create_info;
    create_info.pEnabledFeatures = &device_features;

    const char *device_extensions[1] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    create_info.enabledExtensionCount = 1;
    create_info.ppEnabledExtensionNames = device_extensions;

    if (cfg.validation_layer) {
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers;
    }

    if (vkCreateDevice(physical_device, &create_info, 0, &gpu->device) != VK_SUCCESS) {
        return ERR_fmt(FB_ERR_VK_CREATE_DEVICE, "failed to create device");
    }

    vkGetDeviceQueue(gpu->device, 0, 0, &gpu->graphics_queue);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    struct FbWindow window = {0};
    window.window_handle = glfwCreateWindow(cfg.width, cfg.height, cfg.title, NULL, NULL);
    if (glfwCreateWindowSurface(instance, window.window_handle, 0, &window.surface) != VK_SUCCESS) {
        return ERR_fmt(FB_ERR_VK_CREATE_SURFACE, "failed to create window surface");
    }

    VkBool32 supports_present = false;
    if (vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, 0, window.surface, &supports_present) != VK_SUCCESS)
    {
        return ERR_fmt(FB_ERR_VK_NO_PRESENT, "queue doesnt support present");
    }

    VkSurfaceFormatKHR desired_formats[] = {
        { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
    };
    VkPresentModeKHR desired_modes[] = {
        VK_PRESENT_MODE_MAILBOX_KHR
    };

    VkSurfaceFormatKHR format = {0};
    VkPresentModeKHR mode = {0};
    VkSurfaceCapabilitiesKHR capabilities = {0};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, window.surface, &capabilities);

    WND_get_surface_format(physical_device, window.surface, desired_formats, 1, &format);
    WND_get_present_mode(physical_device, window.surface, desired_modes, 1, &mode);
    VkExtent2D extents = WND_get_swapchain_extents(&capabilities, cfg.width, cfg.height);

    u32 image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
        image_count = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_info = {0};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = window.surface;
    swapchain_info.minImageCount = image_count;
    swapchain_info.imageFormat = format.format;
    swapchain_info.imageColorSpace = format.colorSpace;
    swapchain_info.imageExtent = extents;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.preTransform = capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = mode;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = 0;
    if (vkCreateSwapchainKHR(gpu->device, &swapchain_info, 0, &window.swapchain) != VK_SUCCESS) {
        return ERR_fmt(FB_ERR_VK_CREATE_SWAPCHAIN, "failed to create swapchain");

    }

    vkGetSwapchainImagesKHR(gpu->device, window.swapchain, &window.image_count, 0);
    VkImage *swapchain_images = malloc(image_count * sizeof(*swapchain_images));
    vkGetSwapchainImagesKHR(gpu->device, window.swapchain, &window.image_count, swapchain_images);
    window.swapchain_images = swapchain_images;
    window.swapchain_format = format.format;
    window.swapchain_extent = extents;
    window.swapchain_views = malloc(window.image_count * sizeof(*window.swapchain_views));

    for (u32 i = 0; i < window.image_count; ++i) {
        VkImageViewCreateInfo view_info = {0};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = window.swapchain_images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = window.swapchain_format;
        view_info.components = (VkComponentMapping) {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        };
        view_info.subresourceRange = (VkImageSubresourceRange) {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        };

        if (vkCreateImageView(gpu->device, &view_info, 0, &window.swapchain_views[i]) != VK_SUCCESS) {
            return ERR_fmt(FB_ERR_VK_CREATE_IMAGE_VIEW, "failed to create image view");
        }
    }

    window.buffer_count = image_count;

    VkAttachmentDescription color_attachment = { 0 };
    color_attachment.format = window.swapchain_format;
    color_attachment.samples = 1;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_ref = { 0 };
    color_ref.attachment = 0;
    color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = { 0 };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_ref;

    VkRenderPassCreateInfo renderpass_info = { 0 };
    renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderpass_info.attachmentCount = 1;
    renderpass_info.pAttachments = &color_attachment;
    renderpass_info.subpassCount = 1;
    renderpass_info.pSubpasses = &subpass;

    vkCreateRenderPass(gpu->device, &renderpass_info, 0, &window.swapchain_renderpass);

    VkFramebuffer *swapchain_fbos = malloc(image_count * sizeof(*swapchain_fbos));
    for (u32 i = 0; i < window.buffer_count; ++i) {
        VkFramebufferCreateInfo fbo_info = { 0 };
        fbo_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbo_info.renderPass = window.swapchain_renderpass;
        fbo_info.attachmentCount = 1;
        fbo_info.pAttachments = &window.swapchain_views[i];
        fbo_info.width = window.swapchain_extent.width;
        fbo_info.height = window.swapchain_extent.height;
        fbo_info.layers = 1;

        vkCreateFramebuffer(gpu->device, &fbo_info, 0, &swapchain_fbos[i]);
    }

    window.swapchain_fbos = swapchain_fbos;

    for (u32 i = 0; i < window.buffer_count; ++i) {
        VkSemaphoreCreateInfo sema_info = {0};
        sema_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphore sema = 0;
        vkCreateSemaphore(gpu->device, &sema_info, 0, &sema);
        ARRAY_push(window.image_semaphores, sema);
        vkCreateSemaphore(gpu->device, &sema_info, 0, &sema);
        ARRAY_push(window.finished_semaphores, sema);
    }

    gpu->instance = instance;

    VkCommandPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = 0;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(gpu->device, &pool_info, 0, &gpu->command_pool);

    VkCommandBuffer *command_buffers = malloc(image_count * sizeof(*command_buffers));

    VkCommandBufferAllocateInfo buffer_info = {0};
    buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    buffer_info.commandPool = gpu->command_pool;
    buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    buffer_info.commandBufferCount = image_count;
    vkAllocateCommandBuffers(gpu->device, &buffer_info, command_buffers);

    gpu->command_buffers = command_buffers;

	VmaAllocatorCreateInfo vma_info = {0};
    vma_info.physicalDevice = physical_device;
	vma_info.device = gpu->device;
	//vma_info.preferredLargeHeapBlockSize = MiB(128);

    vmaCreateAllocator(&vma_info, &gpu->allocator);

    vkGetPhysicalDeviceMemoryProperties(physical_device, &gpu->memory_properties);

    window.title = cfg.title;

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

u32 window_acquire_image(struct FbGpu *gpu, struct FbWindow *wnd)
{
    //printf("window_acquire_image: signal=0x%04x\n", wnd->image_semaphores[wnd->current_buffer]);
    vkAcquireNextImageKHR(gpu->device, wnd->swapchain, 0xffffffffffffffff, wnd->image_semaphores[wnd->current_buffer], VK_NULL_HANDLE, &wnd->current_image);

    return wnd->current_buffer;
}

void window_submit(struct FbWindow *wnd, struct FbGpu *gpu, VkCommandBuffer *buffers, u32 buffer_count)
{
    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &wnd->image_semaphores[wnd->current_buffer];
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = buffer_count;
    submit_info.pCommandBuffers = buffers;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &wnd->finished_semaphores[wnd->current_buffer];

    vkQueueSubmit(gpu->graphics_queue, 1, &submit_info, 0);

    //printf("window_submit: wait=0x%04x, signal=%04x\n", wnd->image_semaphores[wnd->current_buffer], wnd->finished_semaphores[wnd->current_buffer]);
}

void window_present(struct FbGpu *gpu, struct FbWindow *wnd)
{
    s32 prev_buffer = wnd->current_image - 1;
    if (prev_buffer < 0) {
        prev_buffer = wnd->buffer_count - 1;
    }

    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &wnd->finished_semaphores[wnd->current_buffer];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &wnd->swapchain;
    present_info.pImageIndices = &wnd->current_image;

    //printf("window_present: wait=0x%04x\n", wnd->finished_semaphores[wnd->current_buffer]);
    vkQueuePresentKHR(gpu->graphics_queue, &present_info);
    vkQueueWaitIdle(gpu->graphics_queue);

    wnd->current_buffer += 1;
    wnd->current_buffer = wnd->current_buffer % wnd->buffer_count;
}

void window_swap(struct FbWindow *wnd)
{
    //glfwSwapBuffers(wnd->window_handle);
}

int window_open(struct FbWindow *wnd)
{
    return !glfwWindowShouldClose(wnd->window_handle);
}

void window_poll(struct FbWindow *_wnd)
{
    glfwPollEvents();

    if (glfwGetKey(_wnd->window_handle, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(_wnd->window_handle, 1);
    }

    s32 w, h;
    glfwGetWindowSize(_wnd->window_handle, &w, &h);

    char buf[128];
    snprintf(buf, sizeof(buf), "%s [w=%d,h=%d]", _wnd->title, w, h);
    glfwSetWindowTitle(_wnd->window_handle, buf);
}

