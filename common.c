#include "common.h"

static const char *VK_LAYER_KHRONOS_validation_name = "VK_LAYER_KHRONOS_validation";

void init_sdl2(void)
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
    // Load instance-level functions (must be after vkCreateInstance!)
    volkLoadInstance(context->instance);

    // Create debug messenger if supported
    if (debugCreateInfo.sType)
    {
        vkCreateDebugUtilsMessengerEXT(context->instance, &debugCreateInfo, NULL, &context->debugUtilsMessenger);
    }
}

void create_sdl2_vulkan_window(MyRenderContext *context, uint32_t flags)
{
    SDL_DisplayMode displayMode;
    int displayIndex = 0;
    int width = INITIAL_WINDOW_WIDTH, height = INITIAL_WINDOW_HEIGHT;
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
        context->isFullscreen = VK_TRUE;
    }

    context->window = SDL_CreateWindow(
        context->sampleName,
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

void create_sdl2_vulkan_instance(MyRenderContext *context, uint32_t flags)
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
    appInfo.pApplicationName = context->sampleName;
    appInfo.apiVersion = VK_API_VERSION_1_3;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    create_vulkan_instance(context, &appInfo, extensions, extCount, flags);
    free(extensions);

    printf("VkInstance successfully created and loaded(%p)\n", (void *)context->instance);
}

void create_sdl2_vulkan_surface(MyRenderContext *context)
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
    uint8_t swapchainSupport = VK_FALSE;

    vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, NULL);
    if (extensionCount == 0)
    {
        return VK_FALSE;
    }

    extensions = malloc(sizeof(VkExtensionProperties) * extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, extensions);

    context->supportedFeatures.swapchainMaintenance1Support = VK_FALSE;
    context->supportedFeatures.dynamicRenderingSupport = VK_FALSE;
    for (uint32_t i = 0; i < extensionCount; i++)
    {
        if (strcmp(extensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
        {
            swapchainSupport = VK_TRUE;
        }
        else if (strcmp(extensions[i].extensionName, VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME) == 0)
        {
            context->supportedFeatures.swapchainMaintenance1Support = VK_TRUE;
        }
        else if (strcmp(extensions[i].extensionName, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME) == 0)
        {
            context->supportedFeatures.dynamicRenderingSupport = VK_TRUE;
        }
    }

    free(extensions);
    return swapchainSupport;
}

void choose_vulkan_physical_device(MyRenderContext *context, uint32_t flags)
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

void  create_vulkan_logical_device(MyRenderContext *context, uint32_t flags)
{
    VkResult r;
    VkDeviceQueueCreateInfo queueInfo[3] = {0};
    VkDeviceCreateInfo deviceInfo = {0};
    //VkPhysicalDeviceFeatures deviceFeatures = {0};
    float defaultQueuePriority = 1.0f;
    uint32_t uniqueQueueFamilyCount = 0;
    const char *enabledExtensions[3] = {0};
    VkPhysicalDevicePresentModeFifoLatestReadyFeaturesEXT presentModeFeatures = {0};
    VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchainMaintenanceFeatures = {0};
    void *pNext = NULL;
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
        presentModeFeatures.pNext = pNext;
        pNext = &presentModeFeatures;
    }

    if (context->supportedFeatures.swapchainMaintenance1Support)
    {
        enabledExtensions[deviceInfo.enabledExtensionCount++] = VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME;
        swapchainMaintenanceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;
        swapchainMaintenanceFeatures.swapchainMaintenance1 = VK_TRUE;
        swapchainMaintenanceFeatures.pNext = pNext;
        pNext = &swapchainMaintenanceFeatures;
    }
    
    deviceInfo.pNext = pNext;

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
    int width = 0, height = 0;

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

    SDL_Vulkan_GetDrawableSize(context->window, &width, &height);
        
    context->swapchainInfo.extent.width = CLAMP((uint32_t)width, surfaceCapabilities.minImageExtent.width, 
        surfaceCapabilities.maxImageExtent.width); 
    context->swapchainInfo.extent.height = CLAMP((uint32_t)height, surfaceCapabilities.minImageExtent.height, 
        surfaceCapabilities.maxImageExtent.height);

    // Swapchain images count
    context->swapchainInfo.imageCount = CLAMP(surfaceCapabilities.minImageCount + 1, surfaceCapabilities.minImageCount,
        surfaceCapabilities.maxImageCount);
    context->swapchainInfo.transformFlags = surfaceCapabilities.currentTransform;
}

void create_vulkan_swapchain(MyRenderContext *context)
{
    VkResult r;
    VkSwapchainKHR oldSwapchain = context->swapchainInfo.swapchain;
    VkSwapchainCreateInfoKHR swapchainInfo = {0};
    VkImage *swapchainImages = NULL;
    VkSemaphoreCreateInfo semaphoreInfo = {0};
    VkFenceCreateInfo fenceInfo = {0};
    VkSwapchainPresentModesCreateInfoEXT presentModesInfo = {0};
    
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
    swapchainInfo.oldSwapchain = oldSwapchain;

    if (context->supportedFeatures.swapchainMaintenance1Support)
    {
        presentModesInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODES_CREATE_INFO_EXT;
        presentModesInfo.presentModeCount = 1;
        presentModesInfo.pPresentModes = &context->presentMode;
        swapchainInfo.pNext = &presentModesInfo;
    }

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

    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

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

        CHECK_VK(vkCreateSemaphore(context->logicalDevice, &semaphoreInfo, NULL, &context->swapchainInfo.framebuffers[i].presentationSemaphore));
        CHECK_VK(vkCreateFence(context->logicalDevice, &fenceInfo, NULL, &context->swapchainInfo.framebuffers[i].presentationCompletedFence));
    }

    // Destroy old swapchain, if exists
    if (oldSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(context->logicalDevice, oldSwapchain, NULL);
    }

    free(swapchainImages);
}

void create_vulkan_command_buffers(MyRenderContext *context)
{
    VkResult r;
    VkCommandPoolCreateInfo commandPoolInfo = {0};
    VkCommandBufferAllocateInfo commandBufferInfo = {0};
    VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];
    VkSemaphoreCreateInfo semaphoreInfo = {0};
    VkFenceCreateInfo fenceInfo = {0};

    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolInfo.queueFamilyIndex = context->graphicsQueue.familyIndex;

    CHECK_VK(vkCreateCommandPool(context->logicalDevice, &commandPoolInfo, NULL, &context->commandPool));

    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferInfo.commandPool = context->commandPool; // one shared command pool for all frames in flight
    commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT; // command buffer for each frame in flight

    CHECK_VK(vkAllocateCommandBuffers(context->logicalDevice, &commandBufferInfo, commandBuffers));

    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        context->framesInFlight[i].commandBuffer = commandBuffers[i];

        CHECK_VK(vkCreateSemaphore(context->logicalDevice, &semaphoreInfo, NULL, &context->framesInFlight[i].imageAvailableSemaphore));
        CHECK_VK(vkCreateFence(context->logicalDevice, &fenceInfo, NULL, &context->framesInFlight[i].submitCompletedFence));
    }
}

static void destroy_vulkan_swapchain_framebuffers(MyRenderContext *context)
{
    VkFence *fences = malloc(sizeof(VkFence) * context->swapchainInfo.imageCount);
    for (uint32_t i = 0; i < context->swapchainInfo.imageCount; i++)
    {
        fences[i] = context->swapchainInfo.framebuffers[i].presentationCompletedFence;
    }

    // wait for all images in swapchain have been presented before destroying swapchain
    vkWaitForFences(context->logicalDevice, context->swapchainInfo.imageCount, fences, VK_TRUE, UINT64_MAX);
    free(fences);

    for (uint32_t i = 0; i < context->swapchainInfo.imageCount; i++)
    {
        vkDestroyFramebuffer(context->logicalDevice, context->swapchainInfo.framebuffers[i].framebuffer, NULL);
        vkDestroyImageView(context->logicalDevice, context->swapchainInfo.framebuffers[i].imageView, NULL);
        vkDestroySemaphore(context->logicalDevice, context->swapchainInfo.framebuffers[i].presentationSemaphore, NULL);
        vkDestroyFence(context->logicalDevice, context->swapchainInfo.framebuffers[i].presentationCompletedFence, NULL);
    }
    
    free(context->swapchainInfo.framebuffers);
    context->swapchainInfo.imageCount = 0;
    context->swapchainInfo.framebuffers = NULL;
}

void destroy_context(MyRenderContext *context)
{
    // wait for the device to finish all executing commands
    vkDeviceWaitIdle(context->logicalDevice);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(context->logicalDevice, context->framesInFlight[i].imageAvailableSemaphore, NULL);
        vkDestroyFence(context->logicalDevice, context->framesInFlight[i].submitCompletedFence, NULL);
    }

    destroy_vulkan_swapchain_framebuffers(context);

    vkDestroyCommandPool(context->logicalDevice, context->commandPool, NULL);
    vkDestroyPipeline(context->logicalDevice, context->graphicsPipeline, NULL);
    vkDestroyPipelineLayout(context->logicalDevice, context->graphicsPipelineLayout, NULL);
    vkDestroyRenderPass(context->logicalDevice, context->renderPass, NULL);
    vkDestroySwapchainKHR(context->logicalDevice, context->swapchainInfo.swapchain, NULL);
    vkDestroyDevice(context->logicalDevice, NULL);
    vkDestroySurfaceKHR(context->instance, context->surface, NULL);
    
    if (context->debugUtilsMessenger) 
    {
        vkDestroyDebugUtilsMessengerEXT(context->instance, context->debugUtilsMessenger, NULL);
    }

    vkDestroyInstance(context->instance, NULL);
    SDL_DestroyWindow(context->window);
    SDL_Quit();
}

void draw_frame(MyRenderContext *context) 
{
    VkResult r;
    void *pNext = NULL;
    MyFrameInFlight *currentFrameInFlight = context->framesInFlight + context->frameStats.frameInFlightIndex;
    VkSubmitInfo submitInfo = {0};
    VkPresentInfoKHR presentInfo = {0};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSwapchainPresentFenceInfoEXT presentFenceInfo = {0};
    VkSwapchainPresentModeInfoEXT presentModeInfo = {0};

    // Wait until all previous render commands owned by the current "frame in flight" have completed 
    vkWaitForFences(context->logicalDevice, 1, &currentFrameInFlight->submitCompletedFence, VK_TRUE, UINT64_MAX);
    vkResetFences(context->logicalDevice, 1, &currentFrameInFlight->submitCompletedFence);

    CHECK_VK(vkAcquireNextImageKHR(context->logicalDevice, context->swapchainInfo.swapchain, UINT64_MAX, 
        currentFrameInFlight->imageAvailableSemaphore, VK_NULL_HANDLE, &currentFrameInFlight->imageIndex));

    // Wait and reset presentation fence if supported
    if (context->supportedFeatures.swapchainMaintenance1Support)
    {
        vkWaitForFences(context->logicalDevice, 1, &context->swapchainInfo.framebuffers[currentFrameInFlight->imageIndex].presentationCompletedFence, VK_TRUE, UINT64_MAX);
        vkResetFences(context->logicalDevice, 1, &context->swapchainInfo.framebuffers[currentFrameInFlight->imageIndex].presentationCompletedFence);
        presentFenceInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT;
        presentFenceInfo.pNext = pNext;
        presentFenceInfo.swapchainCount = 1;
        presentFenceInfo.pFences = &context->swapchainInfo.framebuffers[currentFrameInFlight->imageIndex].presentationCompletedFence;

        presentModeInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODE_INFO_EXT;
        presentModeInfo.pNext = &presentFenceInfo;
        presentModeInfo.swapchainCount = 1;
        presentModeInfo.pPresentModes = &context->presentMode;

        pNext = &presentModeInfo;
    }
    // Reset current command buffer 
    vkResetCommandBuffer(currentFrameInFlight->commandBuffer, 0);

    // Record render commands
    record_render_commands(context, currentFrameInFlight);

    // Submit render commands
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &currentFrameInFlight->imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &currentFrameInFlight->commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &context->swapchainInfo.framebuffers[currentFrameInFlight->imageIndex].presentationSemaphore;
    CHECK_VK(vkQueueSubmit(context->graphicsQueue.queue, 1, &submitInfo, currentFrameInFlight->submitCompletedFence));


    // Present image to the screen
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = pNext;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &context->swapchainInfo.framebuffers[currentFrameInFlight->imageIndex].presentationSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &context->swapchainInfo.swapchain;
    presentInfo.pImageIndices = &currentFrameInFlight->imageIndex;
    CHECK_VK(vkQueuePresentKHR(context->presentQueue.queue, &presentInfo));
}

void update_frame_stats(MyRenderContext *context)
{
    uint64_t currentTimerTick = SDL_GetPerformanceCounter();

    if (context->frameStats.timerFreq == 0)
    {
        context->frameStats.timerFreq = SDL_GetPerformanceFrequency();
        context->frameStats.lastTimerTick = currentTimerTick;
    }

    context->frameStats.frameNumber++;
    context->frameStats.framesPerSecond++;
    // switch to next frame in flight
    context->frameStats.frameInFlightIndex = context->frameStats.frameNumber % MAX_FRAMES_IN_FLIGHT;

    if (currentTimerTick - context->frameStats.lastTimerTick >= context->frameStats.timerFreq)
    {
        context->frameStats.lastTimerTick = currentTimerTick;
        printf("Total frames: %lu, FPS: %lu\n", context->frameStats.frameNumber, context->frameStats.framesPerSecond);
        context->frameStats.framesPerSecond = 0;
    }
}

void resize_sdl2_vulkan_window(MyRenderContext *context)
{
    vkDeviceWaitIdle(context->logicalDevice);
    destroy_vulkan_swapchain_framebuffers(context);
    
    context->isFullscreen = !context->isFullscreen;
    if (context->isFullscreen)
    {
        SDL_SetWindowFullscreen(context->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
    else
    {
        SDL_SetWindowFullscreen(context->window, 0);
        SDL_SetWindowSize(context->window, INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT);
    }

    create_vulkan_swapchain(context);
    SDL_ShowWindow(context->window);
}
