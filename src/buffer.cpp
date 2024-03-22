#include "vulkanApplication.h"

#include "raytracing.hpp"

#include <iostream>
#include <chrono>
#include <random>


/**
 *  Creates uniform buffer objects for each inflight frame.
 */
void VulkanApplication::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(RTParams);

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
 *  Updates the contents of the UBO for the given in-flight frame.
 */
void VulkanApplication::updateUniformBuffer(uint32_t currentImage) {
    // Set up contents of UBO
    RTParams ubo{};
    ubo.deltaTime = totalTime / 1000.f;
    //ubo.deltaTime = lastFrameTime * 2.f; // TODO: UNCOMMENT
    ubo.screenSize = glm::vec2(static_cast<uint32_t>(swapChainExtent.width), static_cast<uint32_t>(swapChainExtent.height));

    // Copy contents into buffer
    // (This is less efficient than using "push constants"
    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}