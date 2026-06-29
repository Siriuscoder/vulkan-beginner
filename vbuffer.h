#pragma once

#include "common.h"

VBuffer create_vulkan_buffer(const MyRenderContext *context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
void copy_vulkan_buffer(const MyRenderContext *context, VBuffer srcBuffer, VBuffer dstBuffer, VkDeviceSize size);
VBuffer create_and_upload_vulkan_buffer(const MyRenderContext *context, const void *bufferData, VkDeviceSize bufferSize, VkBufferUsageFlags usage);
void destroy_vulkan_buffer(const MyRenderContext *context, VBuffer buffer);

VBuffer create_and_upload_vulkan_vbo(const MyRenderContext *context, const void *bufferData, VkDeviceSize bufferSize);
VBuffer create_and_upload_vulkan_ibo(const MyRenderContext *context, const void *bufferData, VkDeviceSize bufferSize);
