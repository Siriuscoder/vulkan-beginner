#include <stdio.h>
#include <stdlib.h>

#include <volk.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#define SAMPLE_FULLSCREEN 0x00000001
#define SAMPLE_VALIDATION_LAYERS 0x00000002
#define SAMPLE_USE_DISCRETE_GPU 0x00000004

#define CHECK_VK(func_call) if ((r = func_call) != VK_SUCCESS) { fprintf(stderr, "Failed '%s': %d\n", #func_call, r); exit(1); }

static const char *sample_name = "Beginner vulkan sample";
static const char *VK_LAYER_KHRONOS_validation_name = "VK_LAYER_KHRONOS_validation";

static void print_extensions(const char **extensions, uint32_t count)
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

static int print_vulkan_instance_extensions(void)
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
    }

    free(extensionsProps);
    return 0;
}

static int check_validation_layers_support(void)
{
    uint32_t layersCount = 0;
    VkResult r;
    VkLayerProperties *layers;
    int ret = 0;

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
            ret = 1;
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
        }

        free(extensionsProps);
    }

    free(layers);
    return ret;
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

static VkInstance create_vulkan_instance_with_validation_layers_support(const VkApplicationInfo *appInfo, 
    const char **extensions, uint32_t extensionsCount)
{
    VkResult r;
    VkInstance instance;
    VkInstanceCreateInfo instInfo;
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    VkValidationFeaturesEXT validationFeatures;
    VkValidationFeatureEnableEXT enables[] = {
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
    };

    // Enabling validation layers logging callback
    memset(&debugCreateInfo, 0, sizeof(debugCreateInfo));
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = validation_layer_debug_callback;

    // Enabling advanced debugging features
    memset(&validationFeatures, 0, sizeof(validationFeatures));
    validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    validationFeatures.pNext = &debugCreateInfo;
    validationFeatures.enabledValidationFeatureCount = sizeof(enables) / sizeof(enables[0]);
    validationFeatures.pEnabledValidationFeatures = enables;

    // Enabling validation layers extensions
    extensions[extensionsCount++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    extensions[extensionsCount++] = VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME;

    memset(&instInfo, 0, sizeof(instInfo));
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pNext = &validationFeatures;
    instInfo.pApplicationInfo = appInfo;
    // Setup extensions
    instInfo.enabledExtensionCount = extensionsCount;
    instInfo.ppEnabledExtensionNames = extensions;
    // Setup validation layers
    instInfo.enabledLayerCount = 1;
    instInfo.ppEnabledLayerNames = &VK_LAYER_KHRONOS_validation_name;

    CHECK_VK(vkCreateInstance(&instInfo, NULL, &instance));
    return instance;
}

static VkInstance create_vulkan_instance(const VkApplicationInfo *appInfo, 
    const char **extensions, uint32_t extensionsCount)
{
    VkResult r;
    VkInstance instance;
    VkInstanceCreateInfo instInfo;

    memset(&instInfo, 0, sizeof(instInfo));
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pApplicationInfo = appInfo;
    // Setup extensions
    instInfo.enabledExtensionCount = extensionsCount;
    instInfo.ppEnabledExtensionNames = extensions;

    CHECK_VK(vkCreateInstance(&instInfo, NULL, &instance));
    return instance;
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

static SDL_Window* create_sdl2_vulkan_window(uint32_t flags)
{
    SDL_DisplayMode displayMode;
    int displayIndex = 0;
    int width = 1024, height = 768;
    uint32_t windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN;
    SDL_Window *vulkanWindow = NULL;

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

    vulkanWindow = SDL_CreateWindow(
        sample_name,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        windowFlags);

    if (!vulkanWindow)
    {
        fprintf(stderr, "Failed to create render window: %s\n", SDL_GetError());
        exit(1);
    }

    printf("Display format: %d bpp, %s, refresh rate %d Hz\n",
        SDL_BITSPERPIXEL(displayMode.format),
        SDL_GetPixelFormatName(displayMode.format),
        displayMode.refresh_rate);

    SDL_ShowWindow(vulkanWindow);
    return vulkanWindow;
}

static VkInstance create_sdl2_vulkan_instance(SDL_Window *window, uint32_t flags)
{
    uint32_t extCount = 0;
    const char **extensions = NULL;
    VkInstance vulkanInstance;
    VkApplicationInfo appInfo;

    // Init volk (global loader)
    if (volkInitialize() != VK_SUCCESS) 
    {
        fprintf(stderr, "Failed to initialize volk\n");
        exit(1);
    }

    print_vulkan_version();
    print_vulkan_instance_extensions();

    if (SDL_Vulkan_GetInstanceExtensions(window, &extCount, NULL) != SDL_TRUE)
    {
        fprintf(stderr, "Failed to get vulkan instance extensions %s\n", SDL_GetError());
        exit(1);
    }

    extensions = malloc(sizeof(char*) * (extCount + 10)); // Reserving some space for validation layers and other extensions
    if (SDL_Vulkan_GetInstanceExtensions(window, &extCount, extensions) != SDL_TRUE)
    {
        fprintf(stderr, "Failed to get vulkan instance extensions %s\n", SDL_GetError());
        exit(1);
    }

    printf("SDL requesting the following extensions:\n");
    print_extensions(extensions, extCount);

    memset(&appInfo, 0, sizeof(VkApplicationInfo));
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = sample_name;
    appInfo.apiVersion = VK_API_VERSION_1_4;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    if (flags & SAMPLE_VALIDATION_LAYERS && check_validation_layers_support())
    {
        vulkanInstance = create_vulkan_instance_with_validation_layers_support(&appInfo, extensions, extCount);
    }
    else
    {
        printf("Validation layers is not supported\n");
        vulkanInstance = create_vulkan_instance(&appInfo, extensions, extCount);
    }
    
    // Load instance-level functions (must be after vkCreateInstance!)
    volkLoadInstance(vulkanInstance);
    free(extensions);

    printf("VkInstance successfully created (%p)\n", (void *)vulkanInstance);
    return vulkanInstance;
}

static VkSurfaceKHR create_sdl2_vulkan_surface(SDL_Window *window, VkInstance instance)
{
    VkSurfaceKHR surface;
    if (SDL_Vulkan_CreateSurface(window, instance, &surface) != SDL_TRUE)
    {
        fprintf(stderr, "Failed to create vulkan surface %s\n", SDL_GetError());
        exit(1);
    }

    return surface;
}

static uint32_t find_situable_queue_family(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    VkResult r;
    uint32_t queueFamilyCount = 0;
    int32_t foundQueueFamilyIndex = UINT32_MAX;

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);

    VkQueueFamilyProperties *queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            VkBool32 presentSupport = 0;
            r = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
            if (r == VK_SUCCESS && presentSupport == VK_TRUE)
            {
                foundQueueFamilyIndex = i;
                break;
            }
        }
    }

    free(queueFamilies);
    return foundQueueFamilyIndex;
}

static VkPhysicalDevice choose_vulkan_physical_device(VkInstance instance, VkSurfaceKHR surface, uint32_t flags)
{
    VkResult r;
    uint32_t deviceCount = 0;
    VkPhysicalDevice chosenDevice = VK_NULL_HANDLE;

    CHECK_VK(vkEnumeratePhysicalDevices(instance, &deviceCount, NULL));
    if (deviceCount == 0)
    {
        fprintf(stderr, "Failed to find GPUs with Vulkan support\n");
        exit(1);
    }

    VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
    CHECK_VK(vkEnumeratePhysicalDevices(instance, &deviceCount, devices));

    printf("Available physical devices:\n");
    for (uint32_t i = 0; i < deviceCount; i++)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);
        printf("\t%s\n", props.deviceName);
    }

    for(uint32_t i = 0; i < deviceCount; i++)
    {
        uint32_t queueFamilyIndex;
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);

        // Check graphics and present queue support
        if ((queueFamilyIndex = find_situable_queue_family(devices[i], surface)) != UINT32_MAX)
        {
            if (flags & SAMPLE_USE_DISCRETE_GPU)
            {
                // Check if discrete GPU
                if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    printf("Using discrete GPU: %s v%d.%d.%d\n", props.deviceName,
                        VK_API_VERSION_MAJOR(props.apiVersion),
                        VK_API_VERSION_MINOR(props.apiVersion),
                        VK_API_VERSION_PATCH(props.apiVersion));

                    chosenDevice = devices[i];
                    break;
                }
            }
            else
            {
                printf("Using first situable GPU: %s v%d.%d.%d\n", props.deviceName,
                    VK_API_VERSION_MAJOR(props.apiVersion),
                    VK_API_VERSION_MINOR(props.apiVersion),
                    VK_API_VERSION_PATCH(props.apiVersion));

                chosenDevice = devices[i];
                break;
            }
        }
    }

    if (chosenDevice == VK_NULL_HANDLE)
    {
        fprintf(stderr, "Failed to find situable GPU\n");
        exit(1);
    }

    free(devices);
    return chosenDevice;
}

static void shutdown(SDL_Window *window, VkInstance instance, VkSurfaceKHR surface)
{
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main()
{
    SDL_Window *window = NULL;
    VkInstance vulkanInstance = VK_NULL_HANDLE;
    VkSurfaceKHR vulkanSurface = VK_NULL_HANDLE;
    VkPhysicalDevice vulkanPhysicalDevice = VK_NULL_HANDLE;
    int8_t running = 1;
    uint32_t flags = 0;

#ifdef VALIDATION_LAYERS
    flags |= SAMPLE_VALIDATION_LAYERS;
#endif

    printf("Starting %s ...\n", sample_name);

    init_sdl2();

    window = create_sdl2_vulkan_window(flags);
    vulkanInstance = create_sdl2_vulkan_instance(window, flags);
    vulkanSurface = create_sdl2_vulkan_surface(window, vulkanInstance);
    vulkanPhysicalDevice = choose_vulkan_physical_device(vulkanInstance, vulkanSurface, flags);

    printf("Chosen physical device: %p\n", (void *)vulkanPhysicalDevice);
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

    shutdown(window, vulkanInstance, vulkanSurface);
    return 0;
}
