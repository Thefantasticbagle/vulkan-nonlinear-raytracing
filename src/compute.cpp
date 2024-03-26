#include "vulkanApplication.h"

#include <iostream>

/**
 *  UBO memory fields.
 */
struct UBOMemory {
    std::vector<VkBuffer>       buffers;
    std::vector<VkDeviceMemory> buffersMemory;
    std::vector<void*>          buffersMapped;
};

/**
 *  All layout-related information required for one pipeline.
 * 
 *  TODO: ADD SSBO AND STORAGE IMAGE
 *  TODO: FIX CLEANUP
 */
struct VKLayout {
    // Layout
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings {};
    std::vector<VkDescriptorPoolSize> poolSizes {};
    std::array<std::vector<VkWriteDescriptorSet>, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)>  descriptorWrites {};

    // Memory
    std::vector<UBOMemory> uboMemories;

    // Calculated properties
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    /**
     *  Adds a pool size to poolSizes.
     *  If the type already exists, increments its descriptorCount instead.
     */
    void addPoolSize( VkDescriptorType type ) {
        for (auto poolSize : poolSizes)
            if (poolSize.type == type) {
                poolSize.descriptorCount += static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
                return;
            }
        poolSizes.push_back(
            VkDescriptorPoolSize{ type, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) }
        );
    }

    /**
     *  Creates and adds a Uniform Buffer Object to the layout.
     *  Allocates memory and binds it to the appropriate fields in uboMemories.
     * 
     *  @param binding Binding, as in shader-code.
     *  @param stageFlags Which stages the UBO should be visible to.
     *  @param physicalDevice The Vulkan physical device.
     *  @param device The Vulkan logical device.
     */
    template<typename T>
    void createUBO (
        uint32_t binding,
        VkShaderStageFlags stageFlags,
        VkPhysicalDevice physicalDevice,
        VkDevice device
    ) {
        // Create and push layout bindings
        layoutBindings.push_back(
            VkDescriptorSetLayoutBinding{ binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, stageFlags, nullptr }
        );

        // Add pool sizes
        addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        // Create buffers
        UBOMemory uboMemory {};
        createUniformBuffers<T>(
            physicalDevice,
            device,
            uboMemory.buffers,
            uboMemory.buffersMemory,
            uboMemory.buffersMapped
        );
        uboMemories.push_back(uboMemory);

        // Create and push descriptor writes
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo *bufferInfo = new VkDescriptorBufferInfo {};
            bufferInfo->buffer = uboMemory.buffers[i];
            bufferInfo->offset = 0;
            bufferInfo->range = sizeof(T);

            VkWriteDescriptorSet write {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstBinding = binding;
            write.dstArrayElement = 0;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.pBufferInfo = bufferInfo;
            
            descriptorWrites[i].push_back(write);
        }
    };

    /**
     *  Creates the Descriptor Set Layout for the layout.
     *  This should be done only once.
     * 
     *  @param device The Vulkan logical device.
     */
    void createDescriptorSetLayout( VkDevice device ) {
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
        layoutInfo.pBindings = layoutBindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("ERR::VULKAN::CREATE_DESCRIPTOR_SET_LAYOUT::CREATION_FAILED");
    }

    /**
     *  Creates the Descriptor Pool for the layout.
     *  This should be done only once.
     * 
     *  @param device The Vulkan logical device.
     */
    void createDescriptorPool( VkDevice device ) {
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
            throw std::runtime_error("ERR::VULKAN::CREATE_DESCRIPTOR_POOL::CREATION_FAILED");
    }

    /**
     *  Creates the Descriptor Sets for the layout.
     *  This should only be done after the Descriptor Pool and Descriptor Set Layout has been created.
     * 
     *  @param device The Vulkan logical device.
     *  
     *  @see VKLayout::createDescriptorSetLayout(...)
     *  @see VKLayout::createDescriptorPool(...)
     */
    void createDescriptorSets( VkDevice device ) {
        // Prepare as many descriptor sets as there are frames-in-flight
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        // Allocate the descriptors
        // (these are automatically destroyed when pool is deleted)
        // (Also, if createDescriptorPool is wrong, this might not give any warnings)
        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        auto err = vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());
        if (err != VK_SUCCESS)
            throw std::runtime_error("ERR::VULKAN::CREATE_DESCRIPTOR_SETS::DESCRIPTOR_SETS_ALLOCATION_FAILED");
    
        // Update descriptor sets
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            for (size_t j = 0; j < descriptorWrites[i].size(); j++)
                descriptorWrites[i][j].dstSet = descriptorSets[i];
            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites[i].size()), descriptorWrites[i].data(), 0, nullptr);
            for (size_t j = 0; j < descriptorWrites[i].size(); j++) // cleanup
                delete descriptorWrites[i][j].pBufferInfo;
        }
    }
};

/**
 *  Creates the descriptor set for buffers.
 */
void VulkanApplication::createComputeDescriptorSetLayout() {
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
    layoutBindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    layoutInfo.pBindings = layoutBindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &computeDescriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::CREATE_COMPUTE_DESCRIPTOR_SET_LAYOUT::CREATION_FAILED");
}

/**
  *  Creates a pool that can allocate as many descriptors for the compute shader as there are in-flight frames.
  */
void VulkanApplication::createComputeDescriptorPool() {
    // Create a pool which contains as many descriptors as there are in-flight frames
    std::array<VkDescriptorPoolSize, 3> poolSizes{};

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    /*
        "Aside from the maximum number of individual descriptors that are available,
        we also need to specify the maximum number of descriptor sets that may be allocated."'
        ? what is the difference
    */
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &computeDescriptorPool) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::CREATE_COMPUTE_DESCRIPTOR_POOL::CREATION_FAILED");
}

/**
 *  Allocates descriptors from the compute shader descriptor pool.
 */
void VulkanApplication::createComputeDescriptorSets() {
    // Prepare as many descriptor sets as there are frames-in-flight
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, computeDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = computeDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    // Allocate the descriptors
    // (these are automatically destroyed when pool is deleted)
    // (Also, if createDescriptorPool is wrong, this might not give any warnings)
    computeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    auto err = vkAllocateDescriptorSets(device, &allocInfo, computeDescriptorSets.data());
    if (err != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::CREATE_COMPUTE_DESCRIPTOR_SETS::DESCRIPTOR_SETS_ALLOCATION_FAILED");

    // Configure UBOs
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Overwrite UBO contents with new buffer
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(RTParams);

        // Configure descriptor writes
        std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = computeDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0; // i = 0
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1; // i = array.count()
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        descriptorWrites[0].pTexelBufferView = nullptr; // Optional

        VkDescriptorBufferInfo storageBufferInfoLastFrame{};
        storageBufferInfoLastFrame.buffer = spheresSSBO[i]; //shaderStorageBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT];
        storageBufferInfoLastFrame.offset = 0;
        storageBufferInfoLastFrame.range = sizeof(RTSphere) * 1;//PARTICLE_COUNT;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = computeDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0; // i = 0
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1; // i = array.count()
        descriptorWrites[1].pBufferInfo = &storageBufferInfoLastFrame;

        VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
        storageBufferInfoCurrentFrame.buffer = blackholesSSBO[i]; //shaderStorageBuffers[i];
        storageBufferInfoCurrentFrame.offset = 0;
        storageBufferInfoCurrentFrame.range = sizeof(RTBlackhole) * 1; //sizeof(RTSphere) * 1;//PARTICLE_COUNT; TODO: MAKE MODULAR

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = computeDescriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0; // i = 0
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1; // i = array.count()
        descriptorWrites[2].pBufferInfo = &storageBufferInfoCurrentFrame;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = textureImageView;

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = computeDescriptorSets[i];
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].dstArrayElement = 0; // i = 0
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrites[3].descriptorCount = 1; // i = array.count()
        descriptorWrites[3].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    /*
        Note: It is possible to have multiple descriptor sets, in that case use this to access them in shader:
            layout(set = 0, binding = 0) uniform UniformBufferObject { ... }
        One example of such a use is for different objects with completely different UBO fields.
    */
}

/**
 *  Creates the compute pipeline.
 */
void VulkanApplication::createComputePipeline() {
	// Load compute shader
	auto compShaderCode = readFile("../resources/shaders/comp.spv");

	// Create shader module
	VkShaderModule  compShaderModule = createShaderModule(compShaderCode);

	// Assign pipeline stages
	VkPipelineShaderStageCreateInfo compShaderStageInfo{};
	compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	compShaderStageInfo.module = compShaderModule;
	compShaderStageInfo.pName = "main"; //entrypoint
	compShaderStageInfo.pSpecializationInfo = nullptr; //can be used to set const values in shader before compilation!

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &computeDescriptorSetLayout;

    VkPushConstantRange computePushConstants = VkPushConstantRange{};
    computePushConstants.offset = 0;
    computePushConstants.size = computePushConstantSize;
    computePushConstants.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    pipelineLayoutInfo.pPushConstantRanges = &computePushConstants;
    pipelineLayoutInfo.pushConstantRangeCount = 1;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("ERR::VULKAN::CREATE_COMPUTE_PIPELINE::PIPELINE_LAYOUT_CREATION_FAILED");

	// Pipeline
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = computePipelineLayout;
    pipelineInfo.stage = compShaderStageInfo;

    //create compute pipeline
	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS)
		throw std::runtime_error("ERR::VULKAN::CREATE_COMPUTE_PIPELINE::PIPELINE_CREATION_FAILED");

	vkDestroyShaderModule(device, compShaderModule, nullptr);
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
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSets[currentFrame], 0, nullptr);

    // TODO: MAKE THIS MODULAR INSTEAD OF FORCED STATIC CAST
    vkCmdPushConstants(commandBuffer, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, computePushConstantSize, (RTFrame*)computePushConstantReference);

    // Dispatch and commit
    //vkCmdDispatch(commandBuffer, static_cast<uint32_t>(swapChainExtent.width), static_cast<uint32_t>(swapChainExtent.height), 1); // TODO: ADD PARTICLE_COUNT VARIABLE
    vkCmdDispatch(commandBuffer, WIDTH / 32, HEIGHT / 32, 1); // TODO: ADD PARTICLE_COUNT VARIABLE
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::RECORD_COMPUTE_COMMAND_BUFFER::COMMIT_FAILED");
}