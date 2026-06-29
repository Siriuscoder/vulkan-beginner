#include "vbuffer.h"

VBuffer create_vulkan_buffer(const MyRenderContext *context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) 
{
    VkResult r;
    VkBufferCreateInfo bufferInfo = {0};
    VkMemoryAllocateInfo allocInfo = {0};
    VBuffer buffer;
    
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    CHECK_VK(vkCreateBuffer(context->logicalDevice, &bufferInfo, NULL, &buffer.buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(context->logicalDevice, buffer.buffer, &memRequirements);

    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = get_vulkan_memory_type_index(context, memRequirements.memoryTypeBits, properties);

    CHECK_VK(vkAllocateMemory(context->logicalDevice, &allocInfo, NULL, &buffer.memory));
    CHECK_VK(vkBindBufferMemory(context->logicalDevice, buffer.buffer, buffer.memory, 0));
    
    buffer.size = size;
    return buffer;
}

void copy_vulkan_buffer(const MyRenderContext *context, VBuffer srcBuffer, VBuffer dstBuffer, VkDeviceSize size) 
{   
    VkResult r;
    VkCommandBufferAllocateInfo allocInfo = {0};
    VkCommandBuffer commandBuffer;
    VkCommandBufferBeginInfo beginInfo = {0};
    VkBufferCopy copyRegion = {0};
    VkSubmitInfo submitInfo = {0};

    SDL_assert(context->transferCommandPool);
    SDL_assert(context->transferQueue.queue);

    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = context->transferCommandPool;
    allocInfo.commandBufferCount = 1;

    CHECK_VK(vkAllocateCommandBuffers(context->logicalDevice, &allocInfo, &commandBuffer));

    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    CHECK_VK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer.buffer, dstBuffer.buffer, 1, &copyRegion);

    CHECK_VK(vkEndCommandBuffer(commandBuffer));

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    CHECK_VK(vkQueueSubmit(context->transferQueue.queue, 1, &submitInfo, VK_NULL_HANDLE));
    CHECK_VK(vkQueueWaitIdle(context->transferQueue.queue));

    vkFreeCommandBuffers(context->logicalDevice, context->transferCommandPool, 1, &commandBuffer);
}

VBuffer create_and_upload_vulkan_buffer(const MyRenderContext *context, const void *bufferData, VkDeviceSize bufferSize, VkBufferUsageFlags usage)
{
    VkResult r;
    void *data = NULL;
    VBuffer deviceBuffer;

    VBuffer stagingBuffer = create_vulkan_buffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    CHECK_VK(vkMapMemory(context->logicalDevice, stagingBuffer.memory, 0, stagingBuffer.size, 0, &data));
    SDL_assert(data);

    memcpy(data, bufferData, (size_t) bufferSize);
    vkUnmapMemory(context->logicalDevice, stagingBuffer.memory);

    deviceBuffer = create_vulkan_buffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copy_vulkan_buffer(context, stagingBuffer, deviceBuffer, bufferSize);
    destroy_vulkan_buffer(context, stagingBuffer);

    return deviceBuffer;
}

void destroy_vulkan_buffer(const MyRenderContext *context, VBuffer buffer) 
{
    vkDestroyBuffer(context->logicalDevice, buffer.buffer, NULL);
    vkFreeMemory(context->logicalDevice, buffer.memory, NULL);
}

VBuffer create_and_upload_vulkan_vbo(const MyRenderContext *context, const void *bufferData, VkDeviceSize bufferSize)
{
    return create_and_upload_vulkan_buffer(context, bufferData, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

VBuffer create_and_upload_vulkan_ibo(const MyRenderContext *context, const void *bufferData, VkDeviceSize bufferSize)
{
    return create_and_upload_vulkan_buffer(context, bufferData, bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}
