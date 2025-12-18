#include <stdio.h>
#include <stdlib.h>

#include <volk.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#define SAMPLE_FULLSCREEN 0x00000001
#define SAMPLE_VALIDATION_LAYERS 0x00000002
#define SAMPLE_USE_DISCRETE_GPU 0x00000004
#define SAMPLE_ENABLE_VSYNC 0x00000005

#define CHECK_VK(func_call) if ((r = func_call) != VK_SUCCESS) { fprintf(stderr, "Failed '%s': %d\n", #func_call, r); exit(1); }

static const char *sample_name = "Beginner vulkan sample";
static const char *VK_LAYER_KHRONOS_validation_name = "VK_LAYER_KHRONOS_validation";

typedef struct VkDeviceFeatures
{
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceLimits limits;
    uint8_t surfaceMaintenance1Support;
    uint8_t getSurfaceCapabilities2Support;
    uint8_t validationLayerSupport;
    uint8_t debugUtilsSupport;
    uint8_t validationFeaturesSupport;
} VkDeviceFeatures;

typedef struct VkQueueInfo
{
    VkQueue queue;
    uint32_t familyIndex;
} VkQueueInfo;

typedef struct VkRenderContext
{
    SDL_Window *window;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    VkSurfaceFormatKHR surfaceFormat;
    VkDeviceFeatures supportedFeatures;
    VkPresentModeKHR presentMode;
    uint32_t queueFamilyCount;
    VkQueueInfo graphicsQueue;
    VkQueueInfo presentQueue;
    VkQueueInfo transferQueue;
} VkRenderContext;

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

static void check_vulkan_instance_extensions_support(VkRenderContext *context)
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

        if (strcmp(extensionsProps[i].extensionName, VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME) == 0)
        {
            context->supportedFeatures.surfaceMaintenance1Support = VK_TRUE;
        }

        if (strcmp(extensionsProps[i].extensionName, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0)
        {
            context->supportedFeatures.getSurfaceCapabilities2Support = VK_TRUE;
        }
    }

    free(extensionsProps);
}

static void check_vulkan_instance_features_support(VkRenderContext *context)
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

static void create_vulkan_instance(VkRenderContext *context, const VkApplicationInfo *appInfo, 
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

static void init_sdl2(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) 
    {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
}

static void create_sdl2_vulkan_window(VkRenderContext *context, uint32_t flags)
{
    SDL_DisplayMode displayMode;
    int displayIndex = 0;
    int width = 1024, height = 768;
    uint32_t windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN;

    if (SDL_GetDesktopDisplayMode(displayIndex, &displayMode) != 0)
    {
        fprintf(stderr, "Failed to get render window display mode: %s\n", SDL_GetError());
        exit(1);
    }

    if (flags & SAMPLE_FULLSCREEN)
    {
        width = displayMode.w;
        height = displayMode.h;

        windowFlags |= SDL_WINDOW_FULLSCREEN;
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

static void create_sdl2_vulkan_instance(VkRenderContext *context, uint32_t flags)
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

static void create_sdl2_vulkan_surface(VkRenderContext *context)
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

static int find_required_queue_families(VkRenderContext *context, VkPhysicalDevice physicalDevice, 
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

static int check_physical_device_formats(VkRenderContext *context, VkPhysicalDevice physicalDevice)
{
    uint32_t formatCount;
    VkSurfaceFormatKHR *surfaceFormats = NULL;

    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, context->surface, &formatCount, NULL);

    if (formatCount == 0) 
    {
        return 0;
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
            return 1;
        }
    }

    free(surfaceFormats);
    return 0;
}

static int check_physical_device_present_modes(VkRenderContext *context, VkPhysicalDevice physicalDevice, uint32_t flags)
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

static int check_physical_device_extensions_support(VkRenderContext *context, VkPhysicalDevice physicalDevice)
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

static void choose_vulkan_physical_device(VkRenderContext *context, uint32_t flags)
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

static void  create_vulkan_logical_device(VkRenderContext *context, uint32_t flags)
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

static void shutdown(VkRenderContext *context)
{
    vkDestroyDevice(context->logicalDevice, NULL);
    vkDestroySurfaceKHR(context->instance, context->surface, NULL);
    vkDestroyInstance(context->instance, NULL);
    SDL_DestroyWindow(context->window);
    SDL_Quit();
}

int main()
{
    int8_t running = 1;
    uint32_t flags = 0;//SAMPLE_ENABLE_VSYNC;
    VkRenderContext context = {0};

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
