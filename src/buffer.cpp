#include "vulkanApplication.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <chrono>
#include <random>


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

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

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
        throw std::runtime_error("ERR::VULKAN::CREATE_DESCRIPTOR_SETS::DESCRIPTOR_SETS_ALLOCATION_FAILED");

    // Configure UBOs
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Overwrite UBO contents with new buffer
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        // Configure descriptor writes
        std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0; // i = 0
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1; // i = array.count()
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        descriptorWrites[0].pTexelBufferView = nullptr; // Optional

        VkDescriptorBufferInfo storageBufferInfoLastFrame{};
        storageBufferInfoLastFrame.buffer = shaderStorageBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT];
        storageBufferInfoLastFrame.offset = 0;
        storageBufferInfoLastFrame.range = sizeof(Particle) * PARTICLE_COUNT;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0; // i = 0
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1; // i = array.count()
        descriptorWrites[1].pBufferInfo = &storageBufferInfoLastFrame;

        VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
        storageBufferInfoCurrentFrame.buffer = shaderStorageBuffers[i];
        storageBufferInfoCurrentFrame.offset = 0;
        storageBufferInfoCurrentFrame.range = sizeof(Particle) * PARTICLE_COUNT;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0; // i = 0
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1; // i = array.count()
        descriptorWrites[2].pBufferInfo = &storageBufferInfoCurrentFrame;

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
    // Set up contents of UBO
    UniformBufferObject ubo{};
    ubo.deltaTime = lastFrameTime * 2.f;

    // Copy contents into buffer
    // (This is less efficient than using "push constants"
    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

/**
 *  Creates the SSBO.
 */
void VulkanApplication::createSSBO() {
    // Initialize particles
    std::default_random_engine rndEngine((unsigned)time(nullptr));
    std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

    // Initial particle positions on a circle
    std::vector<Particle> particles(PARTICLE_COUNT);
    for (auto& particle : particles) {
        float r = 0.25f * sqrt(rndDist(rndEngine));
        float theta = rndDist(rndEngine) * 2.0f * 3.14159265358979323846f;
        float x = r * cos(theta) * HEIGHT / WIDTH;
        float y = r * sin(theta);
        particle.position = glm::vec2(x, y);
        particle.velocity = glm::vec2(0.00005f, 0); //glm::normalize(glm::vec2(x, y)) * 0.00025f;
        particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
    }

    VkDeviceSize bufferSize = sizeof(Particle) * PARTICLE_COUNT;

    // Create a staging buffer used to upload data to the gpu
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, particles.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    shaderStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    shaderStorageBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

    // Copy initial particle data to all storage buffers
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shaderStorageBuffers[i], shaderStorageBuffersMemory[i]);
        copyBuffer(stagingBuffer, shaderStorageBuffers[i], bufferSize);
    }

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}