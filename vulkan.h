#pragma once

#include <volk.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#define CHECK_VK(func_call) if ((r = func_call) != VK_SUCCESS) { fprintf(stderr, "Failed '%s': %d\n", #func_call, r); exit(1); }