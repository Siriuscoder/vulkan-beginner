#pragma once

#include "vulkan.h"

int read_file_to_memory(const char* path, void *buffer, size_t* size);
VkShaderModule load_vulkan_shader_module(VkDevice logicalDevice, const char *filename);