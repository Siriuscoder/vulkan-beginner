#pragma once

#include "volk/volk.h"

#ifdef PLATFORM_Windows
#   include <SDL.h>
#   include <SDL_vulkan.h>
#else
#   include <SDL2/SDL.h>
#   include <SDL2/SDL_vulkan.h>
#endif

#define CHECK_VK(func_call) if ((r = func_call) != VK_SUCCESS) { fprintf(stderr, "Failed '%s': %d\n", #func_call, r); exit(1); }