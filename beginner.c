#include <stdio.h>
#include <stdlib.h>

#include <volk.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#define SAMPLE_FULLSCREEN 0x00000001
#define SAMPLE_VALIDATION_LAYERS 0x00000002

#define CHECK_VK(func_call) if ((r = func_call) != VK_SUCCESS) { fprintf(stderr, "Failed '%s': %d\n", r); exit(1); }

static const char *sample_name = "Beginner vulkan sample";

static void print_extensions(const char **extensions, uint32_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        printf("\t%s\n", extensions[i]);
    }
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

static SDL_Window* create_sdl2_vulkan_window(int flags)
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

static VkInstance init_sdl2_vulkan_instance(SDL_Window *window)
{
    uint32_t extCount = 0;
    const char **extensions = NULL;
    VkInstance vulkanInstance;
    VkApplicationInfo appInfo;
    VkInstanceCreateInfo instInfo;
    VkResult r;

    // Init volk (global loader)
    if (volkInitialize() != VK_SUCCESS) 
    {
        fprintf(stderr, "Failed to initialize volk\n");
        exit(1);
    }

    if (SDL_Vulkan_GetInstanceExtensions(window, &extCount, NULL) != SDL_TRUE)
    {
        fprintf(stderr, "Failed to get vulkan instance extensions %s\n", SDL_GetError());
        exit(1);
    }

    extensions = malloc(sizeof(char*) * extCount);
    if (SDL_Vulkan_GetInstanceExtensions(window, &extCount, extensions) != SDL_TRUE)
    {
        fprintf(stderr, "Failed to get vulkan instance extensions %s\n", SDL_GetError());
        exit(1);
    }

    printf("Required extensions:\n");
    print_extensions(extensions, extCount);

    memset(&instInfo, 0, sizeof(VkInstanceCreateInfo));
    memset(&appInfo, 0, sizeof(VkApplicationInfo));

    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = sample_name;
    appInfo.apiVersion = VK_API_VERSION_1_4;

    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledExtensionCount = extCount;
    instInfo.ppEnabledExtensionNames = extensions;

    CHECK_VK(vkCreateInstance(&instInfo, NULL, &vulkanInstance));
    // Load instance-level functions (must be after vkCreateInstance!)
    volkLoadInstance(vulkanInstance);
    free(extensions);

    return vulkanInstance;
}

static void shutdown(SDL_Window *window, VkInstance instance)
{
    vkDestroyInstance(instance, NULL);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main()
{
    SDL_Window *window = NULL;
    VkInstance vulkanInstance = NULL;
    int8_t running = 1;
    uint32_t flags = 0;

#ifdef VALIDATION_LAYERS
    flags |= SAMPLE_VALIDATION_LAYERS;
#endif

    printf("Starting %s ...\n", sample_name);

    init_sdl2();

    window = create_sdl2_vulkan_window(0);
    vulkanInstance = init_sdl2_vulkan_instance(window);

    printf("VkInstance successfully created (0x%p)\n", vulkanInstance);
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

    shutdown(window, vulkanInstance);
    return 0;
}
