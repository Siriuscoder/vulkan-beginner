#include "shader_io.h"

#include <stdio.h>

int read_file_to_memory(const char* path, void *buffer, size_t* size)
{
    SDL_RWops *handle;
    
    if ((handle = SDL_RWFromFile(path, "rb")) == NULL)
    {
        fprintf(stderr, "Failed to open file %s: %s\n", path, SDL_GetError());
        return VK_FALSE;
    }

    if (!buffer)
    {
        int64_t s = SDL_RWsize(handle);
        if (s < 0)
        {
            fprintf(stderr, "Failed to get file size of %s: %s\n", path, SDL_GetError());
            SDL_RWclose(handle);
            return VK_FALSE;
        }

        *size = s;
        SDL_RWclose(handle);
        return VK_TRUE;
    }


    if (SDL_RWread(handle, buffer, *size, 1) != 1)
    {
        fprintf(stderr, "Failed to read file %s: %s", path, SDL_GetError());
        SDL_RWclose(handle);
        return 0;
    }

    SDL_RWclose(handle);
    return 1;
}

VkShaderModule load_vulkan_shader_module(VkDevice logicalDevice, const char *filename)
{
    VkResult r;
    VkShaderModuleCreateInfo createInfo = {0};
    VkShaderModule shaderModule;
    void *shaderCode;
    size_t shaderSize;

    if (!read_file_to_memory(filename, NULL, &shaderSize))
        exit(1);

    shaderCode = malloc(shaderSize);
    if (!read_file_to_memory(filename, shaderCode, &shaderSize))
        exit(1);

    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderSize;
    createInfo.pCode = shaderCode;

    CHECK_VK(vkCreateShaderModule(logicalDevice, &createInfo, NULL, &shaderModule));
    free(shaderCode);
    return shaderModule;
}
