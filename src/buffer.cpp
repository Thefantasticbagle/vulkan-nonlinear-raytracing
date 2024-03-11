#include "vulkanApplication.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <chrono>

/**
 *  Locates an appropriate format for use in the depth buffer.
 */
VkFormat VulkanApplication::findDepthFormat() {
    return findSupportedFormat(
        /*candidates*/{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        /*tiling*/VK_IMAGE_TILING_OPTIMAL,
        /*features*/VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

/**
 *  Checks for supported features among a list of formats.
 */
VkFormat VulkanApplication::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        // Get properties
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        // Check if properties for linearTiling/optimalTiling are correct
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            return format;

        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return format;
    }

    // None of the candidate formats fulfilled the requirements
    throw std::runtime_error("ERR::VULKAN::FIND_SUPPORTED_FORMAT::NO_SUPPORTED_FORMAT");
}

/**
 *  Creates the image for the depth buffer.
 */
void VulkanApplication::createDepthResources() {
    // Select format
    VkFormat depthFormat = findDepthFormat();

    // Create image and view
    createImage(
        swapChainExtent.width,
        swapChainExtent.height,
        1,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        depthImage,
        depthImageMemory
    );

    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    // Transition image format (this does not need to be done, but is done to be more explicit)
    transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

/**
 *  Reads, allocates, and creates an example texture.
 */
void VulkanApplication::createTextureImage() {
    // Load image from file
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("../resources/textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels)
        throw std::runtime_error("ERR::VULKAN::CREATE_TEXTURE_IMAGE::STBI_LOAD_FAILURE");

    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    // Create staging buffer and transfer data
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory
    );

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    // Create and allocate image
    createImage(
        texWidth,
        texHeight,
        mipLevels,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        textureImage,
        textureImageMemory
    );

    // Transition the receiving image's format to a copy-optimised one and copy the stagingbuffer to it
    // Note: The VK_IMAGE_LAYOUT_GENERAL format supports all operations and can be good for testing, but is not optimal.
    transitionImageLayout(
        textureImage,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        mipLevels
    );

    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    // Lastly, transition the receiving image's format to a sample-optimised one
    // FOR NOW; UNUSED DUE TO MIPMAPPING.
    //transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);
    generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);

    // Cleanup
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

/**
 *  Creates the view for the texture.
 */
void VulkanApplication::createTextureImageView() {
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
}

/**
 *  Creates the sampler for the texture.
 */
void VulkanApplication::createTextureSampler() {
    // Create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f; // To see effects: try static_cast<float>(mipLevels / 2);
    samplerInfo.maxLod = static_cast<float>(mipLevels);
    samplerInfo.mipLodBias = 0.0f; // Optional

    //set up anisotropic filtering
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; // Color of outside of texture dimensions
    samplerInfo.unnormalizedCoordinates = VK_FALSE; // True -> [0,width) and [0,height) instead of [0,1)

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
        throw std::runtime_error("failed to create texture sampler!");
}

/**
 *  Creates the VBO.
 */
void VulkanApplication::createVertexBuffer() {
    // Create staging buffer
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        /*size*/bufferSize,
        /*usage*/VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        // VK_BUFFER_USAGE_TRANSFER_SRC_BIT - can be used as source for memory transfer.
        /*properties*/VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        /*buffer*/stagingBuffer,
        /*bufferMemory*/stagingBufferMemory
    );

    // Transfer vertices data to staging buffer
    /*
        Since VK_MEMORY_PROPERTY_HOST_COHERENT_BIT is used, there is no need to worry about async problems.
        Otherwise, vkFlushMappedMemoryRanges / vkInvalidateMappedMemoryRanges must be called after writing / before reading memory.
        Latter may lead to better performance.

        Even then, the data might not be visible for the GPU immidiately.
        However, it's guaranteed to be so by the next call to vkQueueSubmit.
    */
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize); // transfer vertices data to mapped memory
    vkUnmapMemory(device, stagingBufferMemory);

    // Create VBO which is optimized for GPU and transfer memory from staging buffer
    createBuffer(
        /*size*/bufferSize,
        // VK_BUFFER_USAGE_TRANSFER_DST_BIT - can be used as destination for memory transfer.
        /*usage*/VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        /*properties*/VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        /*buffer*/vertexBuffer,
        /*bufferMemory*/vertexBufferMemory
    );

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    // Cleanup
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

/**
 *  Creates the EBO/IBO.
 */
void VulkanApplication::createIndexBuffer() {
    // Create staging buffer
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        /*size*/bufferSize,
        /*usage*/VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        // VK_BUFFER_USAGE_TRANSFER_SRC_BIT - can be used as source for memory transfer.
        /*properties*/VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        /*buffer*/stagingBuffer,
        /*bufferMemory*/stagingBufferMemory
    );

    // Transfer indices data to staging buffer
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)bufferSize); // transfer vertices data to mapped memory
    vkUnmapMemory(device, stagingBufferMemory);

    // Create EBO/IBO which is optimized for GPU and transfer memory from staging buffer
    createBuffer(
        /*size*/bufferSize,
        // VK_BUFFER_USAGE_TRANSFER_DST_BIT - can be used as destination for memory transfer.
        /*usage*/VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        /*properties*/VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        /*buffer*/indexBuffer,
        /*bufferMemory*/indexBufferMemory
    );

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    // Cleanup
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

/**
 *  Creates uniform buffer objects for each inflight frame.
 */
void VulkanApplication::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    // Create space for buffers
    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    // Create and map buffers
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            uniformBuffers[i],
            uniformBuffersMemory[i]
        );

        vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

/**
  *  Creates a pool that can allocate as many UBO descriptors as there are in-flight frames.
  */
void VulkanApplication::createDescriptorPool() {
    // Create a pool which contains as many descriptors as there are in-flight frames
    std::array<VkDescriptorPoolSize, 2> poolSizes{};

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

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

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::CREATE_DESCRIPTOR_POOL::CREATION_FAILED");
}

/**
 *  Allocates UBO descriptors from the pool.
 */
void VulkanApplication::createDescriptorSets() {
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
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate descriptor sets!");

    // Configure UBOs
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Overwrite UBO contents with new buffer
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        // Bind image and sampler to descriptors in descriptor set
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        // Configure descriptor writes
        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0; // i = 0
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1; // i = array.count()
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        descriptorWrites[0].pTexelBufferView = nullptr; // Optional

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0; // i = 0
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1; // i = array.count()
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    /*
        Note: It is possible to have multiple descriptor sets, in that case use this to access them in shader:
            layout(set = 0, binding = 0) uniform UniformBufferObject { ... }
        One example of such a use is for different objects with completely different UBO fields.
    */
}

/**
 *  Updates the contents of the UBO for the given in-flight frame.
 */
void VulkanApplication::updateUniformBuffer(uint32_t currentImage) {
    // Measure time
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    // Set up contents of UBO
    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1; // Since GLM uses inverse Y clip coordinates, this must be done to flip it back

    // Copy contents into buffer
    // (This is less efficient than using "push constants"
    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}