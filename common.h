#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "vulkan.h"

#ifndef MIN
#   define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
#   define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef CLAMP
#   define CLAMP(value, min, max) MIN(MAX(value, min), max)
#endif

#define SAMPLE_FULLSCREEN           0x00000001
#define SAMPLE_VALIDATION_LAYERS    0x00000002
#define SAMPLE_USE_DISCRETE_GPU     0x00000004
#define SAMPLE_ENABLE_VSYNC         0x00000008

#define INITIAL_WINDOW_WIDTH        1024
#define INITIAL_WINDOW_HEIGHT       768

#define MAX_FRAMES_IN_FLIGHT        2

typedef struct MyDeviceFeatures
{
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceLimits limits;
    uint8_t surfaceMaintenance1Support;
    uint8_t getSurfaceCapabilities2Support;
    uint8_t validationLayerSupport;
    uint8_t debugUtilsSupport;
    uint8_t validationFeaturesSupport;
    uint8_t swapchainMaintenance1Support;
} MyDeviceFeatures;

typedef struct MyQueueInfo
{
    VkQueue queue;
    uint32_t familyIndex;
} MyQueueInfo;

typedef struct MySwapchainFramebuffer
{
    VkImage image;
    VkImageView imageView;
    VkFramebuffer framebuffer;
    VkSemaphore presentationSemaphore;
    VkFence presentationCompletedFence;
} MySwapchainFramebuffer;

typedef struct MySwapchainInfo
{
    VkExtent2D extent;
    VkSurfaceTransformFlagBitsKHR transformFlags;
    VkSwapchainKHR swapchain;
    uint32_t imageCount;
    MySwapchainFramebuffer *framebuffers;
} MySwapchainInfo;

typedef struct MyFrameStats
{
    uint64_t timerFreq;
    uint64_t lastTimerTick;
    uint64_t framesPerSecond;
    uint64_t frameNumber;
    uint32_t frameInFlightIndex;
} MyFrameStats;

typedef struct MyFrameInFlight
{
    VkSemaphore imageAvailableSemaphore;
    VkFence submitCompletedFence;
    VkCommandBuffer commandBuffer;
    uint32_t imageIndex;
} MyFrameInFlight;

typedef struct MyRenderContext
{
    const char *sampleName;
    SDL_Window *window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugUtilsMessenger;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    VkSurfaceFormatKHR surfaceFormat;
    MyDeviceFeatures supportedFeatures;
    VkPresentModeKHR presentMode;
    MySwapchainInfo swapchainInfo;
    uint32_t queueFamilyCount;
    MyQueueInfo graphicsQueue;
    MyQueueInfo presentQueue;
    MyQueueInfo transferQueue;
    VkRenderPass renderPass;
    VkPipelineLayout graphicsPipelineLayout;
    VkPipeline graphicsPipeline;
    VkCommandPool commandPool;
    MyFrameStats frameStats;
    MyFrameInFlight framesInFlight[MAX_FRAMES_IN_FLIGHT];
    uint8_t isFullscreen;
} MyRenderContext;

void record_render_commands(MyRenderContext *context, MyFrameInFlight *frameInFlight);

#include "shader_io.h"

void init_sdl2(void);
void create_sdl2_vulkan_window(MyRenderContext *context, uint32_t flags);
void create_sdl2_vulkan_instance(MyRenderContext *context, uint32_t flags);
void create_sdl2_vulkan_surface(MyRenderContext *context);
void choose_vulkan_physical_device(MyRenderContext *context, uint32_t flags);
void create_vulkan_logical_device(MyRenderContext *context, uint32_t flags);
void create_vulkan_swapchain(MyRenderContext *context);
void create_vulkan_command_buffers(MyRenderContext *context);
void draw_frame(MyRenderContext *context);
void update_frame_stats(MyRenderContext *context);
void resize_sdl2_vulkan_window(MyRenderContext *context);
void destroy_context(MyRenderContext *context);