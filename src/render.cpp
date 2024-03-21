#include "vulkanApplication.h"

#include <array>
#include <iostream>

/**
 *  Creates render pass object.
 *  This object is required to tell Vulkan which framebuffer attachments will be used for rendering, number of color/depth buffers, samples, etc.
 */
void VulkanApplication::createRenderPass() {
    // Color attachment descriptor
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // Unless multisampling is used, leave as 1
    
    // What to do with data in attachment before/after rendering
    // (For now, store ops here to render triangle on screen)
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // VK_ATTACHMENT_LOAD_OP_LOAD / VK_ATTACHMENT_LOAD_OP_CLEAR / VK_ATTACHMENT_LOAD_OP_DONT_CARE
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // VK_ATTACHMENT_STORE_OP_STORE / VK_ATTACHMENT_STORE_OP_DONT_CARE

    // What to do with stencil data
    // (For now, we dont care)
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // Layout of pixels in memory, some common examples include:
    //      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL - Color attachment
    //      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR          - Presented in swap chain
    //      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL     - Used as destination for memory copy operation
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;      // Layout of image before render pass
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // After -=-

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0; // layout(location = 0) out vec4 outColor
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Set up subpasses
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    // pInputAttachments - read from shader
    // pResolveAttachments - used for multisampling color attachments
    // pDepthStencilAttachment - depth/stencil data
    // pPreserveAttachments - not used by THIS subpass, but data should be preserved

    // Create render subpass dependencies
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    //wait for color attachment output stage
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Create render pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::CREATE_RENDER_PASS::CREATION_FAILED");
}

/**
 *  Creates the descriptor set for buffers.
 */
void VulkanApplication::createDescriptorSetLayout() {
    std::array<VkDescriptorSetLayoutBinding, 4> layoutBindings{};
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindings[0].pImmutableSamplers = nullptr;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[1].pImmutableSamplers = nullptr;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    layoutBindings[2].binding = 2;
    layoutBindings[2].descriptorCount = 1;
    layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[2].pImmutableSamplers = nullptr;
    layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    layoutBindings[3].binding = 3;
    layoutBindings[3].descriptorCount = 1;
    layoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    layoutBindings[3].pImmutableSamplers = nullptr;
    layoutBindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // TODO: ADD SAMPLER
    /*layoutBindings[4].binding = 4;
    layoutBindings[4].descriptorCount = 1;
    layoutBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindings[4].pImmutableSamplers = nullptr;
    layoutBindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;*/

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    layoutInfo.pBindings = layoutBindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::CREATE_DESCRIPTOR_SET_LAYOUT::CREATION_FAILED");
}

/**
 *  Creates the graphics pipeline.
 *  Also sets up the Fixed-function state (input assembly, rasterizer, viewport, scissoring, color blending, etc.
 */
void VulkanApplication::createGraphicsPipeline() {
    // Read shader code
    auto vertShaderCode = readFile("../resources/shaders/vert.spv");
    auto fragShaderCode = readFile("../resources/shaders/frag.spv");

    // Create shader modules
    // (Since the SPIR-V code is not made into machine code until the pipeline is created, these objects can be local and deleted)
    VkShaderModule  vertShaderModule = createShaderModule(vertShaderCode),
                    fragShaderModule = createShaderModule(fragShaderCode);

    // Assign shader pipeline stages to each of the modules
    //vertex shader
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main"; //entrypoint
    vertShaderStageInfo.pSpecializationInfo = nullptr; //can be used to set const values in shader before compilation!

    //fragment shader
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    // Add pipeline stages to array
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // Set up vertex input state
    // (For now, there are no inputs to the vertex shader)
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    // Set up input assembly
    // (For now, simple triangles)
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Set up viewport
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f; // [0, 1]
    viewport.maxDepth = 1.0f; // [0, 1]

    // Scissor (area of image to include)
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    // Make viewport and scissor a dynamic part of the pipeline, this does not cause much performance loss
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // Specify their count at pipeline creation time
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Set up rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE; // False -> disregards frags outside of depth area. Should be True for shadow maps.
    rasterizer.rasterizerDiscardEnable = VK_FALSE; // True -> No rasterization

    rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // VK_POLYGON_MODE_FILL / (VK_POLYGON_MODE_LINE / VK_POLYGON_MODE_POINT)
    rasterizer.lineWidth = 1.0f; // > 1 requires wideLines GPU feature
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    // Depth bias is not necessary for this case. Can be useful for biasing depth based on i.e. slope.
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    // Set up anti-aliasing (disabled for now...)
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    // Set up depth and stencil testing
    // (Disabled for now)

    // Set up color blending (see VkBlendFactor and VkBlendOp enums for all ops)
    //per-framebuffer settings
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    //global settings
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Why can we use multiple layout descriptors??
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::CREATE_GRAPHICS_PIPELINE::PIPELINE_LAYOUT_CREATION_FAILED");

    // Finally fill up pipeline structure with objects made so far!
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    //VkPipelineShaderStageCreateInfo structs
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;

    //fixed-function
    pipelineInfo.layout = pipelineLayout;

    //pipeline layout
    //see https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap8.html#renderpass-compatibility for compatibility
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0; // index of subpass where graphics pipeline is used

    //derivative pipelines (so far unused)
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    //create graphics pipeline
    if (vkCreateGraphicsPipelines(
        device,
        VK_NULL_HANDLE, // VkPipelineCache object for reusing data between pipeline creations
        1,
        &pipelineInfo,
        nullptr,
        &graphicsPipeline) != VK_SUCCESS
    )
        throw std::runtime_error("ERR::VULKAN::CREATE_GRAPHICS_PIPELINE::CREATION_FAILED");

    // Cleanup
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

/**
 *  Wraps shader code into a VkShaderModule object.
 */
VkShaderModule VulkanApplication::createShaderModule(const std::vector<char>& code) {
    // Create info object
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    // Create shader module and return
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::CREATE_SHADER_MODULE::CREATION_FAILED");

    return shaderModule;
}

/**
 *  Re records a given command buffer with an image in the swapchain.
 */
void VulkanApplication::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    // Supply details about the usage of this specific command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT      - Record after executing once.
    // VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT - Secondary command buffer which is entirely contained by a single render pass.
    // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT     - Can be resubmitted WHILE pending execution.
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::RECORD_COMMAND_BUFFER::COMMAND_BUFFER_BEGIN_FAILED");

    // Start render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex]; // Which image in swapchain to draw to
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;

    // Set which clear values VK_ATTACHMENT_LOAD_OP_CLEAR will use
    // Needs to be in the same order as attachments!
    std::array<VkClearValue, 1> clearValues;
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;
    renderPassInfo.pClearValues = clearValues.data();

    // Begin render pass
    vkCmdBeginRenderPass(
        commandBuffer,
        &renderPassInfo,
        // VK_SUBPASS_CONTENTS_INLINE - Embed commands into primary controll buffer (no secondary command buffer executed)
        // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS - Execute commands from secondary command buffer.
        VK_SUBPASS_CONTENTS_INLINE
    );

    // Bind pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &shaderStorageBuffers[currentFrame], offsets);

    // Draw
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    // End render pass
    vkCmdEndRenderPass(commandBuffer);

    // Finish recording to command buffer
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::RECORD_COMMAND_BUFFER::COMMIT_FAILED");
}

/**
 *  Records the command buffer for Compute.
 */
void VulkanApplication::recordComputeCommandBuffer(VkCommandBuffer commandBuffer) {
    // Supply details about the usage of this specific command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT      - Record after executing once.
    // VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT - Secondary command buffer which is entirely contained by a single render pass.
    // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT     - Can be resubmitted WHILE pending execution.
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::RECORD_COMPUTE_COMMAND_BUFFER::COMMAND_BUFFER_BEGIN_FAILED");

    // Bind pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

    // Dispatch and commit
    vkCmdDispatch(commandBuffer, 256, 256, 1); // TODO: ADD PARTICLE_COUNT VARIABLE
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::RECORD_COMPUTE_COMMAND_BUFFER::COMMIT_FAILED");
}