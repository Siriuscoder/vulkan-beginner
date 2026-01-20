#include "common.h"

static const char *sample_name = "Dynamic render vulkan sample";

void create_vulkan_pipeline(MyRenderContext *context)
{
    VkResult r;
    VkPipelineShaderStageCreateInfo shaderStages[2] = {0};
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {0};
    VkPipelineViewportStateCreateInfo viewportStateInfo = {0};
    VkPipelineRasterizationStateCreateInfo rasterizerInfo = {0};
    VkPipelineMultisampleStateCreateInfo multisamplingInfo = {0};
    VkPipelineColorBlendAttachmentState colorBlendAttachmentInfo = {0};
    VkPipelineColorBlendStateCreateInfo colorBlendingInfo = {0};
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {0};
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {0};

    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = load_vulkan_shader_module(context->logicalDevice, "shaders/base.vert.spv");
    shaderStages[0].pName = "main"; // Entry point name

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = load_vulkan_shader_module(context->logicalDevice, "shaders/base.frag.spv");
    shaderStages[1].pName = "main"; // Entry point name

    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo.viewportCount = 1;
    viewportStateInfo.scissorCount = 1;

    rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerInfo.depthClampEnable = VK_FALSE;
    rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerInfo.lineWidth = 1.0f;
    rasterizerInfo.cullMode = VK_CULL_MODE_NONE;
    rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizerInfo.depthBiasEnable = VK_FALSE;

    multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingInfo.sampleShadingEnable = VK_FALSE;
    multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    colorBlendAttachmentInfo.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentInfo.blendEnable = VK_FALSE;

    colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendingInfo.logicOpEnable = VK_FALSE;
    colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendingInfo.attachmentCount = 1;
    colorBlendingInfo.pAttachments = &colorBlendAttachmentInfo;
    colorBlendingInfo.blendConstants[0] = 0.0f;
    colorBlendingInfo.blendConstants[1] = 0.0f;
    colorBlendingInfo.blendConstants[2] = 0.0f;
    colorBlendingInfo.blendConstants[3] = 0.0f;

    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState);
    dynamicStateInfo.pDynamicStates = dynamicStates;

    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    CHECK_VK(vkCreatePipelineLayout(context->logicalDevice, &pipelineLayoutInfo, NULL, &context->graphicsPipelineLayout));

    pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = &context->surfaceFormat.format;

    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &pipelineRenderingCreateInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportStateInfo;
    pipelineInfo.pRasterizationState = &rasterizerInfo;
    pipelineInfo.pMultisampleState = &multisamplingInfo;
    pipelineInfo.pColorBlendState = &colorBlendingInfo;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = context->graphicsPipelineLayout;

    CHECK_VK(vkCreateGraphicsPipelines(context->logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &context->graphicsPipeline));

    vkDestroyShaderModule(context->logicalDevice, shaderStages[0].module, NULL);
    vkDestroyShaderModule(context->logicalDevice, shaderStages[1].module, NULL);
}

void record_render_commands(MyRenderContext *context, MyFrameInFlight *frameInFlight)
{
    VkResult r;
    VkRenderingAttachmentInfo renderingAttachment = {0};
    VkRenderingInfo renderingInfo = {0};
    VkImageMemoryBarrier2 imageLayoutBarrier = {0};
    VkDependencyInfo dependencyInfo = {0};
    VkCommandBufferBeginInfo bufferBeginInfo = {0};
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    VkViewport viewport = {0};
    VkRect2D scissor = {0};

    // Describe render attachment
    renderingAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    renderingAttachment.imageView = context->swapchainInfo.framebuffers[frameInFlight->imageIndex].imageView;
    renderingAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    renderingAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    renderingAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    renderingAttachment.clearValue = clearColor;

    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset.x = 0;
    renderingInfo.renderArea.offset.y = 0;
    renderingInfo.renderArea.extent = context->swapchainInfo.extent;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &renderingAttachment;

    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // Image layout transition barrier, undefined -> color attachment optimal
    imageLayoutBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    imageLayoutBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageLayoutBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imageLayoutBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_NONE; // no matter
    imageLayoutBarrier.srcAccessMask = VK_ACCESS_2_NONE_KHR;
    imageLayoutBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    imageLayoutBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
    imageLayoutBarrier.image = context->swapchainInfo.framebuffers[frameInFlight->imageIndex].image;
    imageLayoutBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageLayoutBarrier.subresourceRange.levelCount = 1;
    imageLayoutBarrier.subresourceRange.layerCount = 1;

    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &imageLayoutBarrier;

    // Viewport and scissor parameters
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)renderingInfo.renderArea.extent.width;
    viewport.height = (float)renderingInfo.renderArea.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    scissor.offset = renderingInfo.renderArea.offset;
    scissor.extent = renderingInfo.renderArea.extent;

    // start recording render commands
    CHECK_VK(vkBeginCommandBuffer(frameInFlight->commandBuffer, &bufferBeginInfo));
    // Image layout transition barrier, undefined -> color attachment optimal
    vkCmdPipelineBarrier2(frameInFlight->commandBuffer, &dependencyInfo);
    // begin render pass
    vkCmdBeginRendering(frameInFlight->commandBuffer, &renderingInfo);
    // set viewport
    vkCmdSetViewport(frameInFlight->commandBuffer, 0, 1, &viewport);
    // set scissor
    vkCmdSetScissor(frameInFlight->commandBuffer, 0, 1, &scissor);
    // bind pipeline, bind shaders 
    vkCmdBindPipeline(frameInFlight->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->graphicsPipeline);
    // draw batch 
    vkCmdDraw(frameInFlight->commandBuffer, 3, 1, 0, 0);
    // end render pass
    vkCmdEndRendering(frameInFlight->commandBuffer);

    imageLayoutBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imageLayoutBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    imageLayoutBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    imageLayoutBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    imageLayoutBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_NONE; // not matter
    imageLayoutBarrier.dstAccessMask = VK_ACCESS_2_NONE_KHR;
    // Image layout transition barrier, color attachment optimal -> present source
    vkCmdPipelineBarrier2(frameInFlight->commandBuffer, &dependencyInfo);
    // end recording render commands
    CHECK_VK(vkEndCommandBuffer(frameInFlight->commandBuffer));
}

int main(int argc, char **argv)
{
    int8_t running = VK_TRUE;
    uint32_t flags = SAMPLE_ENABLE_VSYNC;
    MyRenderContext context = {0};
    SDL_Event e;

    context.sampleName = sample_name;
#ifdef VALIDATION_LAYERS
    flags |= SAMPLE_VALIDATION_LAYERS;
#endif

    printf("Starting %s ...\n", context.sampleName);

    init_sdl2();

    create_sdl2_vulkan_window(&context, flags);
    create_sdl2_vulkan_instance(&context, flags);
    create_sdl2_vulkan_surface(&context);
    choose_vulkan_physical_device(&context, flags);
    create_vulkan_logical_device(&context, flags);
    create_vulkan_swapchain(&context);
    create_vulkan_pipeline(&context);
    create_vulkan_command_buffers(&context);

    printf("Press escape to quit\n");

    while (running)
    {
        draw_frame(&context);
        update_frame_stats(&context);

        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                {
                    running = VK_FALSE;
                }
                else if (e.key.keysym.sym == SDLK_f)
                {
                    resize_sdl2_vulkan_window(&context);
                }
            }
        }
    }

    destroy_context(&context);
    return 0;
}
