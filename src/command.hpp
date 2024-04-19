#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>


/**
 *  Creates a command pool.
 * 
 *  @param physicalDevice The Vulkan physical device.
 *  @param device The Vulkan logical device.
 *  @param queueFamilyIndex Index for the queue family which is used.
 *  @param commandPool Empty variable for the command pool.
 */
void inline createCommandPool(
    VkPhysicalDevice    physicalDevice,
    VkDevice            device,
    uint32_t            queueFamilyIndex,
    VkCommandPool       & commandPool
) {
    //QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT - Hint that command buffers are re recorded with new commands very often.
    // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT - Allow command buffers to be re recorded individually, otherwise they must be reset together.
    // Since we want to record a command buffer each frame and reset a specific one, use the latter.
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndex;//queueFamilyIndices.graphicsAndComputeFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::CREATE_COMMAND_POOL::CREATION_FAILED");
}

/**
 *  Creates a one-time-use command-buffer.
 * 
 *  @param device The Vulkan logical device.
 *  @param commandPool Command pool to pull commands from.
 * 
 *  @return A command buffer for single time use.
 */
VkCommandBuffer inline beginSingleTimeCommands(
    VkDevice        device,
    VkCommandPool   commandPool
) {
    // Set up command buffer
    // Note: It is better to have a separate commandPool for this so that Vulkan may optimize memory allocation.
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    // Record commands
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

/**
 *  Ends and commits a command-buffer.
 * 
 *  @param commandBuffer The command buffer.
 *  @param commandPool The command pool to free commands from.
 *  @param device The Vulkan logical device.
 *  @param queue The queue to submit commands to.
 */
void inline endSingleTimeCommands(
    VkCommandBuffer commandBuffer,
    VkCommandPool   commandPool,
    VkDevice        device,
    VkQueue         queue
) {
    vkEndCommandBuffer(commandBuffer);

    // Submit commands to graphics queue and WAIT
    // Note: It would be much better to use a fence to wait so that sequential buffer copies don't have to wait for each other.
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkResult res = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    if (res != VK_SUCCESS) throw std::runtime_error("ERR::VULKAN::END_SINGLE_TIME_COMMANDS::UNEXPECTED_SUBMIT_ERROR");
    res = vkQueueWaitIdle(queue);
//    if (res != VK_SUCCESS) 
//        throw std::runtime_error("ERR::VULKAN::END_SINGLE_TIME_COMMANDS::UNEXPECTED_WAIT_ERROR");

    // Cleanup
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}