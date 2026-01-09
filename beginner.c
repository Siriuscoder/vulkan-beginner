#include <stdio.h>
#include <stdlib.h>

#include <volk.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

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

#define CHECK_VK(func_call) if ((r = func_call) != VK_SUCCESS) { fprintf(stderr, "Failed '%s': %d\n", #func_call, r); exit(1); }

static const char *sample_name = "Beginner vulkan sample";
static const char *VK_LAYER_KHRONOS_validation_name = "VK_LAYER_KHRONOS_validation";

typedef struct MyDeviceFeatures
{
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceLimits limits;
    uint8_t surfaceMaintenance1Support;
    uint8_t getSurfaceCapabilities2Support;
    uint8_t validationLayerSupport;
    uint8_t debugUtilsSupport;
    uint8_t validationFeaturesSupport;
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
} MySwapchainFramebuffer;

typedef struct MySwapchainInfo
{
    VkExtent2D extent;
    VkSurfaceTransformFlagBitsKHR transformFlags;
    VkSwapchainKHR swapchain;
    uint32_t imageCount;
    MySwapchainFramebuffer *framebuffers;
} MySwapchainInfo;

typedef struct MyRenderContext
{
    SDL_Window *window;
    VkInstance instance;
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
} MyRenderContext;

static void init_sdl2(void)
{
#ifdef SDL_HINT_APP_NAME
    SDL_SetHint(SDL_HINT_APP_NAME, "Vulkan beginner sample"); 
#endif

#ifdef SDL_HINT_WINDOWS_DPI_AWARENESS
    // "permonitorv2": Request per-monitor V2 DPI awareness. (Windows 10, version 1607 and later). 
    // The most visible difference from "permonitor" is that window title bar will be scaled to the visually
    // correct size when dragging between monitors with different scale factors. This is the preferred DPI awareness level.
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#endif

#ifdef SDL_HINT_VIDEO_HIGHDPI_DISABLED
    // Allow HiDPI
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) 
    {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
}

static void print_extensions(const char *extensions[], uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        printf("\t%s\n", extensions[i]);
    }
}

static void print_vulkan_version(void)
{
    VkResult r;
    uint32_t version;

    CHECK_VK(vkEnumerateInstanceVersion(&version));
    printf("Vulkan version: %d.%d.%d\n", 
        VK_API_VERSION_MAJOR(version), 
        VK_API_VERSION_MINOR(version), 
        VK_API_VERSION_PATCH(version));
}

static void check_vulkan_instance_extensions_support(MyRenderContext *context)
{
    VkResult r;
    uint32_t extensionCount = 0;
    VkExtensionProperties *extensionsProps;

    CHECK_VK(vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL));
    extensionsProps = malloc(sizeof(VkExtensionProperties) * extensionCount);
    CHECK_VK(vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensionsProps));
    printf("Available instance extensions:\n");
    for (uint32_t i = 0; i < extensionCount; i++)
    {
        printf("\t%s v%d.%d.%d\n", extensionsProps[i].extensionName, 
            VK_API_VERSION_MAJOR(extensionsProps[i].specVersion),
            VK_API_VERSION_MINOR(extensionsProps[i].specVersion),
            VK_API_VERSION_PATCH(extensionsProps[i].specVersion));

#ifdef VK_KHR_surface_maintenance1
        if (strcmp(extensionsProps[i].extensionName, VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME) == 0)
        {
            context->supportedFeatures.surfaceMaintenance1Support = VK_TRUE;
        }
#endif

#ifdef VK_KHR_get_surface_capabilities2
        if (strcmp(extensionsProps[i].extensionName, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0)
        {
            context->supportedFeatures.getSurfaceCapabilities2Support = VK_TRUE;
        }
    }
#endif

    free(extensionsProps);
}

static void check_vulkan_instance_features_support(MyRenderContext *context)
{
    uint32_t layersCount = 0;
    VkResult r;
    VkLayerProperties *layers;

    check_vulkan_instance_extensions_support(context);

    CHECK_VK(vkEnumerateInstanceLayerProperties(&layersCount, NULL));
    layers = malloc(sizeof(VkLayerProperties) * layersCount);
    CHECK_VK(vkEnumerateInstanceLayerProperties(&layersCount, layers));

    printf("Available layers:\n");
    for (uint32_t i = 0; i < layersCount; i++)
    {
        uint32_t extensionCount = 0;
        VkExtensionProperties *extensionsProps;

        if (strcmp(layers[i].layerName, VK_LAYER_KHRONOS_validation_name) == 0)
        {
            context->supportedFeatures.validationLayerSupport = VK_TRUE;
        }

        CHECK_VK(vkEnumerateInstanceExtensionProperties(layers[i].layerName, &extensionCount, NULL));

        extensionsProps = malloc(sizeof(VkExtensionProperties) * extensionCount);
        CHECK_VK(vkEnumerateInstanceExtensionProperties(layers[i].layerName, &extensionCount, extensionsProps));

        printf("\t%s v%d.%d.%d (%d) Layer extensions:\n", layers[i].layerName, 
            VK_API_VERSION_MAJOR(layers[i].specVersion), 
            VK_API_VERSION_MINOR(layers[i].specVersion), 
            VK_API_VERSION_PATCH(layers[i].specVersion), 
            layers[i].implementationVersion);

        for (uint32_t j = 0; j < extensionCount; j++)
        {
            printf("\t\t%s v%d.%d.%d\n", extensionsProps[j].extensionName, 
                VK_API_VERSION_MAJOR(extensionsProps[j].specVersion), 
                VK_API_VERSION_MINOR(extensionsProps[j].specVersion), 
                VK_API_VERSION_PATCH(extensionsProps[j].specVersion));

            if (strcmp(extensionsProps[j].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
            {
                context->supportedFeatures.debugUtilsSupport = VK_TRUE;
            }

            if (strcmp(extensionsProps[j].extensionName, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME) == 0)
            {
                context->supportedFeatures.validationFeaturesSupport = VK_TRUE;
            }
        }

        free(extensionsProps);
    }

    free(layers);
}

static VkBool32 validation_layer_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
    VkDebugUtilsMessageTypeFlagsEXT messageType, 
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
{
    printf("VALIDATION %s(%s): %s\n", (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ? "ERROR" : 
        (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ? "WARNING" :
        (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT ? "INFO" :
        (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT ? "VERBOSE" : "UNKNOWN")))),
        pCallbackData->pMessageIdName,
        pCallbackData->pMessage);

    return VK_FALSE;
}

static void create_vulkan_instance(MyRenderContext *context, const VkApplicationInfo *appInfo, 
    const char **extensions, uint32_t extensionsCount, uint32_t flags)
{
    VkResult r;
    void *pNext = NULL;
    VkInstanceCreateInfo instInfo = {0};
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {0};
    VkValidationFeaturesEXT validationFeatures = {0};
    VkValidationFeatureEnableEXT enables[] = {
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
    };

    if (flags & SAMPLE_VALIDATION_LAYERS)
    {
        if (context->supportedFeatures.validationLayerSupport)
        {
            if (context->supportedFeatures.debugUtilsSupport)
            {
                // Enabling validation layers logging callback
                debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
                debugCreateInfo.pNext = pNext;
                debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                debugCreateInfo.pfnUserCallback = validation_layer_debug_callback;
                debugCreateInfo.pUserData = NULL;

                extensions[extensionsCount++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
                pNext = &debugCreateInfo;
            }

            if (context->supportedFeatures.validationFeaturesSupport)
            {
                // Enabling advanced debugging features
                validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
                validationFeatures.pNext = pNext;
                validationFeatures.enabledValidationFeatureCount = sizeof(enables) / sizeof(enables[0]);
                validationFeatures.pEnabledValidationFeatures = enables;

                extensions[extensionsCount++] = VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME;
                pNext = &validationFeatures;
            }

            // Setup validation layers
            instInfo.enabledLayerCount = 1;
            instInfo.ppEnabledLayerNames = &VK_LAYER_KHRONOS_validation_name;
        }
        else
        {
            printf("Validation layers requested but not supported\n");
        }
    }

    if (context->supportedFeatures.surfaceMaintenance1Support)
    {
        extensions[extensionsCount++] = VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME;
    }

    if (context->supportedFeatures.getSurfaceCapabilities2Support)
    {
        extensions[extensionsCount++] = VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME;
    }

    printf("Activating the following extensions:\n");
    print_extensions(extensions, extensionsCount);

    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pNext = pNext;
    instInfo.pApplicationInfo = appInfo;
    // Setup extensions
    instInfo.enabledExtensionCount = extensionsCount;
    instInfo.ppEnabledExtensionNames = extensions;

    CHECK_VK(vkCreateInstance(&instInfo, NULL, &context->instance));
}

static void create_sdl2_vulkan_window(MyRenderContext *context, uint32_t flags)
{
    SDL_DisplayMode displayMode;
    int displayIndex = 0;
    int width = 1024, height = 768;
    uint32_t windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI;

    if (SDL_GetDesktopDisplayMode(displayIndex, &displayMode) != 0)
    {
        fprintf(stderr, "Failed to get render window display mode: %s\n", SDL_GetError());
        exit(1);
    }

    if (flags & SAMPLE_FULLSCREEN)
    {
        width = displayMode.w;
        height = displayMode.h;

        windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        windowFlags |= SDL_WINDOW_BORDERLESS;
    }

    context->window = SDL_CreateWindow(
        sample_name,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        windowFlags);

    if (!context->window)
    {
        fprintf(stderr, "Failed to create render window: %s\n", SDL_GetError());
        exit(1);
    }

    printf("Display format: %d bpp, %s, refresh rate %d Hz\n",
        SDL_BITSPERPIXEL(displayMode.format),
        SDL_GetPixelFormatName(displayMode.format),
        displayMode.refresh_rate);

    SDL_ShowWindow(context->window);
}

static void create_sdl2_vulkan_instance(MyRenderContext *context, uint32_t flags)
{
    uint32_t extCount = 0;
    const char **extensions = NULL;
    VkApplicationInfo appInfo = {0};

    // Init volk (global loader)
    if (volkInitialize() != VK_SUCCESS) 
    {
        fprintf(stderr, "Failed to initialize volk\n");
        exit(1);
    }

    print_vulkan_version();
    check_vulkan_instance_features_support(context);

    if (SDL_Vulkan_GetInstanceExtensions(context->window, &extCount, NULL) != SDL_TRUE)
    {
        fprintf(stderr, "Failed to get vulkan instance extensions %s\n", SDL_GetError());
        exit(1);
    }

    extensions = malloc(sizeof(char*) * (extCount + 10)); // Reserving some space for validation layers and other extensions
    if (SDL_Vulkan_GetInstanceExtensions(context->window, &extCount, extensions) != SDL_TRUE)
    {
        fprintf(stderr, "Failed to get vulkan instance extensions %s\n", SDL_GetError());
        exit(1);
    }

    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = sample_name;
    appInfo.apiVersion = VK_API_VERSION_1_3;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    create_vulkan_instance(context, &appInfo, extensions, extCount, flags);
    free(extensions);

    // Load instance-level functions (must be after vkCreateInstance!)
    volkLoadInstance(context->instance);
    printf("VkInstance successfully created and loaded(%p)\n", (void *)context->instance);
}

static void create_sdl2_vulkan_surface(MyRenderContext *context)
{
    if (SDL_Vulkan_CreateSurface(context->window, context->instance, &context->surface) != SDL_TRUE)
    {
        fprintf(stderr, "Failed to create vulkan surface %s\n", SDL_GetError());
        exit(1);
    }
}

static VkQueueFamilyProperties *get_device_supported_queue_families(VkPhysicalDevice physicalDevice, uint32_t *queueFamilyCount)
{
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, queueFamilyCount, NULL);
    VkQueueFamilyProperties *queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * *queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, queueFamilyCount, queueFamilies);
    return queueFamilies;
}

static int find_required_queue_families(MyRenderContext *context, VkPhysicalDevice physicalDevice, 
    const VkQueueFamilyProperties *queueFamilies, uint32_t queueFamilyCount)
{
    VkResult r;
    uint32_t foundGraphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t foundPresentQueueFamilyIndex = UINT32_MAX;
    uint32_t foundTransferQueueFamilyIndex = UINT32_MAX;
    uint32_t *queuesCounter = calloc(queueFamilyCount, sizeof(uint32_t));

    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        VkBool32 presentSupport = 0;
        if (foundGraphicsQueueFamilyIndex == UINT32_MAX && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            if (queuesCounter[i] < queueFamilies[i].queueCount)
            {
                queuesCounter[i]++;
                foundGraphicsQueueFamilyIndex = i;
            }
        }
        
        if (foundTransferQueueFamilyIndex == UINT32_MAX && queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            if (queuesCounter[i] < queueFamilies[i].queueCount)
            {
                queuesCounter[i]++;
                foundTransferQueueFamilyIndex = i;
            }
        }

        if (foundPresentQueueFamilyIndex == UINT32_MAX)
        {
            r = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, context->surface, &presentSupport);
            if (r == VK_SUCCESS && presentSupport == VK_TRUE)
            {
                if (queuesCounter[i] < queueFamilies[i].queueCount)
                {
                    queuesCounter[i]++;
                    foundPresentQueueFamilyIndex = i;
                }
            }
        }

        if (foundGraphicsQueueFamilyIndex != UINT32_MAX &&
            foundPresentQueueFamilyIndex != UINT32_MAX &&
            foundTransferQueueFamilyIndex != UINT32_MAX)
        {
            context->graphicsQueue.familyIndex = foundGraphicsQueueFamilyIndex;
            context->presentQueue.familyIndex = foundPresentQueueFamilyIndex;
            context->transferQueue.familyIndex = foundTransferQueueFamilyIndex;
            free(queuesCounter);
            return VK_TRUE;
        }
    }

    free(queuesCounter);
    return VK_FALSE;
}

static int check_physical_device_formats(MyRenderContext *context, VkPhysicalDevice physicalDevice)
{
    uint32_t formatCount;
    VkSurfaceFormatKHR *surfaceFormats = NULL;

    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, context->surface, &formatCount, NULL);

    if (formatCount == 0) 
    {
        return VK_FALSE;
    }

    surfaceFormats = malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, context->surface, &formatCount, surfaceFormats);

    for (uint32_t i = 0; i < formatCount; i++)
    {
        if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            context->surfaceFormat = surfaceFormats[i];
            free(surfaceFormats);
            return VK_TRUE;
        }
    }

    free(surfaceFormats);
    return VK_FALSE;
}

static int check_physical_device_present_modes(MyRenderContext *context, VkPhysicalDevice physicalDevice, uint32_t flags)
{
    uint32_t presentModeCount;
    VkPresentModeKHR *presentModes = NULL;

    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, context->surface, &presentModeCount, NULL);
    if (presentModeCount == 0)
    {
        return VK_FALSE;
    }

    presentModes = malloc(sizeof(VkPresentModeKHR) * presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, context->surface, &presentModeCount, presentModes);

    if (flags & SAMPLE_ENABLE_VSYNC)
    {
        // If VSYNC is enabled, use VK_PRESENT_MODE_MAILBOX_KHR by default if it is supported
        // VK_PRESENT_MODE_MAILBOX_KHR is the best choise for triple buffering
        for (uint32_t i = 0; i < presentModeCount; i++)
        {
            if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                context->presentMode = presentModes[i];
                free(presentModes);
                return VK_TRUE;
            }
        }

        // If VK_PRESENT_MODE_MAILBOX_KHR is not supported, try VK_PRESENT_MODE_FIFO_LATEST_READY_EXT
        for (uint32_t i = 0; i < presentModeCount; i++)
        {
            if (presentModes[i] == VK_PRESENT_MODE_FIFO_LATEST_READY_EXT)
            {
                context->presentMode = presentModes[i];
                free(presentModes);
                return VK_TRUE;
            }
        }

        free(presentModes);
        // If VK_PRESENT_MODE_FIFO_LATEST_READY_KHR is not supported, use VK_PRESENT_MODE_FIFO_KHR, it must be supported
        context->presentMode = VK_PRESENT_MODE_FIFO_KHR;
        return VK_TRUE;
    }

    // No sync mode, try VK_PRESENT_MODE_IMMEDIATE_KHR
    for (uint32_t i = 0; i < presentModeCount; i++)
    {
        if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            context->presentMode = presentModes[i];
            free(presentModes);
            return VK_TRUE;
        }
    }

    free(presentModes);
    return VK_FALSE;
}

static int check_physical_device_extensions_support(MyRenderContext *context, VkPhysicalDevice physicalDevice)
{
    uint32_t extensionCount;
    VkExtensionProperties *extensions = NULL;

    vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, NULL);
    if (extensionCount == 0)
    {
        return VK_FALSE;
    }

    extensions = malloc(sizeof(VkExtensionProperties) * extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, extensions);

    for (uint32_t i = 0; i < extensionCount; i++)
    {
        if (strcmp(extensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
        {
            free(extensions);
            return VK_TRUE;
        }
    }

    free(extensions);
    return VK_FALSE;
}

static void choose_vulkan_physical_device(MyRenderContext *context, uint32_t flags)
{
    VkResult r;
    uint32_t deviceCount = 0;
    VkPhysicalDevice *devices = NULL;

    CHECK_VK(vkEnumeratePhysicalDevices(context->instance, &deviceCount, NULL));
    if (deviceCount == 0)
    {
        fprintf(stderr, "Failed to find GPUs with Vulkan support\n");
        exit(1);
    }

    devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
    CHECK_VK(vkEnumeratePhysicalDevices(context->instance, &deviceCount, devices));

    for (uint32_t i = 0; i < deviceCount; i++)
    {
        uint32_t queueFamilyCount = 0;
        VkQueueFamilyProperties *queueFamilies = NULL;
        VkPhysicalDeviceProperties props;
        VkPhysicalDeviceFeatures features;

        vkGetPhysicalDeviceProperties(devices[i], &props);
        vkGetPhysicalDeviceFeatures(devices[i], &features);

        queueFamilies = get_device_supported_queue_families(devices[i], &queueFamilyCount);
        if (!find_required_queue_families(context, devices[i], queueFamilies, queueFamilyCount))
        {
            free(queueFamilies);
            continue;
        }

        free(queueFamilies);
        if (!check_physical_device_formats(context, devices[i]))
        {
            continue;
        }

        if (!check_physical_device_present_modes(context, devices[i], flags))
        {
            continue;
        }

        if (!check_physical_device_extensions_support(context, devices[i]))
        {
            continue;
        }

        context->queueFamilyCount = queueFamilyCount;
        context->supportedFeatures.features = features;
        context->supportedFeatures.limits = props.limits;

        if (flags & SAMPLE_USE_DISCRETE_GPU)
        {
            // Check if discrete GPU
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                context->physicalDevice = devices[i];
                printf("Selected discrete GPU: %s\n", props.deviceName);
                break;
            }
        }
        else
        {
            context->physicalDevice = devices[i];
            printf("Selected first suitable GPU: %s\n", props.deviceName);
            break;
        }
    }

    if (context->physicalDevice == VK_NULL_HANDLE)
    {
        fprintf(stderr, "Failed to find situable GPU\n");
        exit(1);
    }

    free(devices);
}

static void  create_vulkan_logical_device(MyRenderContext *context, uint32_t flags)
{
    VkResult r;
    VkDeviceQueueCreateInfo queueInfo[3] = {0};
    VkDeviceCreateInfo deviceInfo = {0};
    //VkPhysicalDeviceFeatures deviceFeatures = {0};
    float defaultQueuePriority = 1.0f;
    uint32_t uniqueQueueFamilyCount = 0;
    const char *enabledExtensions[2] = {0};
    VkPhysicalDevicePresentModeFifoLatestReadyFeaturesEXT presentModeFeatures = {0};
    uint32_t *queueFamilyIndex = calloc(context->queueFamilyCount, sizeof(uint32_t));
    
    queueFamilyIndex[context->graphicsQueue.familyIndex]++;
    queueFamilyIndex[context->presentQueue.familyIndex]++;
    queueFamilyIndex[context->transferQueue.familyIndex]++;

    for (uint32_t i = 0; i < context->queueFamilyCount; i++)
    {
        if (queueFamilyIndex[i] > 0)
        {
            queueInfo[uniqueQueueFamilyCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo[uniqueQueueFamilyCount].queueFamilyIndex = i;
            queueInfo[uniqueQueueFamilyCount].pQueuePriorities = &defaultQueuePriority;
            queueInfo[uniqueQueueFamilyCount].queueCount = queueFamilyIndex[i];
            uniqueQueueFamilyCount++;
        }
    }
    
    // Create logical device
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = uniqueQueueFamilyCount;
    deviceInfo.pQueueCreateInfos = queueInfo;
    // VkPhysicalDeviceFeatures and other nested structs may be setup via pNext chain of the VkDeviceCreateInfo
    //deviceInfo.pEnabledFeatures = &deviceFeatures;
    if (flags & SAMPLE_VALIDATION_LAYERS && context->supportedFeatures.validationLayerSupport)
    {
        deviceInfo.enabledLayerCount = 1;
        deviceInfo.ppEnabledLayerNames = &VK_LAYER_KHRONOS_validation_name;
    }

    deviceInfo.ppEnabledExtensionNames = enabledExtensions;
    enabledExtensions[deviceInfo.enabledExtensionCount++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

    if (context->presentMode == VK_PRESENT_MODE_FIFO_LATEST_READY_EXT)
    {
        enabledExtensions[deviceInfo.enabledExtensionCount++] = VK_EXT_PRESENT_MODE_FIFO_LATEST_READY_EXTENSION_NAME;
        presentModeFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_MODE_FIFO_LATEST_READY_FEATURES_EXT;
        presentModeFeatures.presentModeFifoLatestReady = VK_TRUE;
        deviceInfo.pNext = &presentModeFeatures;
    }
    
    printf("Activating the following device extensions:\n");
    print_extensions(enabledExtensions, deviceInfo.enabledExtensionCount);
    CHECK_VK(vkCreateDevice(context->physicalDevice, &deviceInfo, NULL, &context->logicalDevice));
    // load device functions
    volkLoadDevice(context->logicalDevice);

    // Get queues
    vkGetDeviceQueue(context->logicalDevice, context->graphicsQueue.familyIndex, 
        --queueFamilyIndex[context->graphicsQueue.familyIndex], &context->graphicsQueue.queue);

    vkGetDeviceQueue(context->logicalDevice, context->presentQueue.familyIndex, 
        --queueFamilyIndex[context->presentQueue.familyIndex], &context->presentQueue.queue);

    vkGetDeviceQueue(context->logicalDevice, context->transferQueue.familyIndex, 
        --queueFamilyIndex[context->transferQueue.familyIndex], &context->transferQueue.queue);

    SDL_assert(context->graphicsQueue.queue && context->presentQueue.queue && context->transferQueue.queue);
    free(queueFamilyIndex);
}

static void retrieve_vulkan_swapchain_info(MyRenderContext *context)
{
    VkResult r;
    VkSurfaceCapabilitiesKHR surfaceCapabilities = {0};

#if defined(VK_KHR_surface_maintenance1) && defined(VK_KHR_get_surface_capabilities2)
    // Advanced device capabilities queries
    if (context->supportedFeatures.surfaceMaintenance1Support &&
        context->supportedFeatures.getSurfaceCapabilities2Support)
    {
        VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {0};
        VkSurfaceCapabilities2KHR surfaceCapabilities2 = {0};
        VkSurfacePresentModeKHR presentMode = {0};

        surfaceCapabilities2.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;

        presentMode.sType = VK_STRUCTURE_TYPE_SURFACE_PRESENT_MODE_KHR;
        presentMode.presentMode = context->presentMode;

        surfaceInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
        surfaceInfo.pNext = &presentMode;
        surfaceInfo.surface = context->surface;

        CHECK_VK(vkGetPhysicalDeviceSurfaceCapabilities2KHR(context->physicalDevice, &surfaceInfo, &surfaceCapabilities2));
        surfaceCapabilities = surfaceCapabilities2.surfaceCapabilities;
    }
    else
#endif
    {
        CHECK_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physicalDevice, context->surface, &surfaceCapabilities));
    }

    if (surfaceCapabilities.currentExtent.width == UINT32_MAX || 
        surfaceCapabilities.currentExtent.height == UINT32_MAX)
    {
        int width = 0, height = 0;
        SDL_Vulkan_GetDrawableSize(context->window, &width, &height);
        
        context->swapchainInfo.extent.width = CLAMP((uint32_t)width, surfaceCapabilities.minImageExtent.width, 
            surfaceCapabilities.maxImageExtent.width); 
        context->swapchainInfo.extent.height = CLAMP((uint32_t)height, surfaceCapabilities.minImageExtent.height, 
            surfaceCapabilities.maxImageExtent.height);
    }
    else
    {
        context->swapchainInfo.extent = surfaceCapabilities.currentExtent;
    }

    // Swapchain images count
    context->swapchainInfo.imageCount = CLAMP(surfaceCapabilities.minImageCount + 1, surfaceCapabilities.minImageCount,
        surfaceCapabilities.maxImageCount);
    context->swapchainInfo.transformFlags = surfaceCapabilities.currentTransform;
}

static void create_vulkan_swapchain(MyRenderContext *context)
{
    VkResult r;
    VkSwapchainCreateInfoKHR swapchainInfo = {0};
    VkImage *swapchainImages = NULL;
    
    retrieve_vulkan_swapchain_info(context);

    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = context->surface;
    swapchainInfo.minImageCount = context->swapchainInfo.imageCount;
    swapchainInfo.imageFormat = context->surfaceFormat.format;
    swapchainInfo.imageColorSpace = context->surfaceFormat.colorSpace;
    swapchainInfo.imageExtent = context->swapchainInfo.extent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (context->graphicsQueue.familyIndex != context->presentQueue.familyIndex) 
    {
        uint32_t queueFamilyIndices[] = {context->graphicsQueue.familyIndex, context->presentQueue.familyIndex};

        swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainInfo.queueFamilyIndexCount = 2;
        swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    swapchainInfo.preTransform = context->swapchainInfo.transformFlags;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = context->presentMode;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

    // Create swapchain
    CHECK_VK(vkCreateSwapchainKHR(context->logicalDevice, &swapchainInfo, NULL, &context->swapchainInfo.swapchain));
    // Retrieve swapchain images count, may not be the same as requested
    CHECK_VK(vkGetSwapchainImagesKHR(context->logicalDevice, context->swapchainInfo.swapchain, 
        &context->swapchainInfo.imageCount, NULL));

    // Retrieve swapchain images
    context->swapchainInfo.framebuffers = calloc(context->swapchainInfo.imageCount, sizeof(MySwapchainFramebuffer));
    swapchainImages = malloc(context->swapchainInfo.imageCount * sizeof(VkImage));
    CHECK_VK(vkGetSwapchainImagesKHR(context->logicalDevice, context->swapchainInfo.swapchain, 
        &context->swapchainInfo.imageCount, swapchainImages));

    // Create image views and framebuffers for each swapchain image
    for (uint32_t i = 0; i < context->swapchainInfo.imageCount; i++)
    {
        VkImageViewCreateInfo createImageInfo = {0};
        VkFramebufferCreateInfo createFramebufferInfo = {0};

        createImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createImageInfo.image = swapchainImages[i];
        createImageInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createImageInfo.format = context->surfaceFormat.format;
        createImageInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createImageInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createImageInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createImageInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createImageInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createImageInfo.subresourceRange.baseMipLevel = 0;
        createImageInfo.subresourceRange.levelCount = 1;
        createImageInfo.subresourceRange.baseArrayLayer = 0;
        createImageInfo.subresourceRange.layerCount = 1;

        CHECK_VK(vkCreateImageView(context->logicalDevice, &createImageInfo, NULL, &context->swapchainInfo.framebuffers[i].imageView));
        context->swapchainInfo.framebuffers[i].image = swapchainImages[i];

        createFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createFramebufferInfo.renderPass = context->renderPass;
        createFramebufferInfo.attachmentCount = 1;
        createFramebufferInfo.pAttachments = &context->swapchainInfo.framebuffers[i].imageView;
        createFramebufferInfo.width = context->swapchainInfo.extent.width;
        createFramebufferInfo.height = context->swapchainInfo.extent.height;
        createFramebufferInfo.layers = 1;

        CHECK_VK(vkCreateFramebuffer(context->logicalDevice, &createFramebufferInfo, NULL, 
            &context->swapchainInfo.framebuffers[i].framebuffer));
    }

    free(swapchainImages);
}

static void create_vulkan_render_pass(MyRenderContext *context)
{
    VkResult r;
    VkAttachmentDescription colorAttachment = {0};
    VkAttachmentReference colorAttachmentRef = {0};
    VkSubpassDescription subpass = {0};
    VkSubpassDependency dependency = {0};
    VkRenderPassCreateInfo renderPassInfo = {0};

    colorAttachment.format = context->surfaceFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Subpass attachment refering to color attachment 0
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    CHECK_VK(vkCreateRenderPass(context->logicalDevice, &renderPassInfo, NULL, &context->renderPass));
}

static void shutdown(MyRenderContext *context)
{
    for (uint32_t i = 0; i < context->swapchainInfo.imageCount; i++)
    {
        vkDestroyFramebuffer(context->logicalDevice, context->swapchainInfo.framebuffers[i].framebuffer, NULL);
        vkDestroyImageView(context->logicalDevice, context->swapchainInfo.framebuffers[i].imageView, NULL);
    }

    free(context->swapchainInfo.framebuffers);

    vkDestroyRenderPass(context->logicalDevice, context->renderPass, NULL);
    vkDestroySwapchainKHR(context->logicalDevice, context->swapchainInfo.swapchain, NULL);
    vkDestroyDevice(context->logicalDevice, NULL);
    vkDestroySurfaceKHR(context->instance, context->surface, NULL);
    vkDestroyInstance(context->instance, NULL);
    SDL_DestroyWindow(context->window);
    SDL_Quit();
}

int main()
{
    int8_t running = 1;
    uint32_t flags = SAMPLE_ENABLE_VSYNC;
    MyRenderContext context = {0};

#ifdef VALIDATION_LAYERS
    flags |= SAMPLE_VALIDATION_LAYERS;
#endif

    printf("Starting %s ...\n", sample_name);

    init_sdl2();

    create_sdl2_vulkan_window(&context, flags);
    create_sdl2_vulkan_instance(&context, flags);
    create_sdl2_vulkan_surface(&context);
    choose_vulkan_physical_device(&context, flags);
    create_vulkan_logical_device(&context, flags);
    create_vulkan_render_pass(&context);
    create_vulkan_swapchain(&context);

    printf("Press any key to quit\n");

    while (running)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e)) 
        {
            if (e.type == SDL_KEYDOWN)
                running = 0;
        }

        SDL_Delay(10);
    }

    shutdown(&context);
    return 0;
}
