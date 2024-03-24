#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "command.hpp"

#include <stdexcept>
#include <vector>


/**
 *  Locates memory of a given type and props.
 */
uint32_t inline findMemoryType(
    uint32_t                typeFilter,
    VkMemoryPropertyFlags   properties,
    VkPhysicalDevice        physicalDevice
) {
    // Fetch memory props
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    /*
        We only care about memoryTypes, not the memoryHeaps.
        Although memoryHeaps can be used to select based on type of memory (VRAM vs RAM/swapspace).

        each memoryType has property flags which include:
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT - CPU can map and read/write to memory.
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT - Mapped data is immidiately unmappable, no need to worry about waiting for it.
    */

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        // Pick first viable memory type
        // (check if bits in typeFilter are set to "1" for the given memorytype)
        // (verify that the correct properties are present)
        if ( typeFilter & (1 << i) &&
             (memProperties.memoryTypes[i].propertyFlags & properties) == properties
        ) return i;
    }

    throw std::runtime_error("ERR::VULKAN::FIND_MEMORY_TYPE::NONE_FOUND");
}

/**
 *  Creates a buffer.
 *
 *  @param size The size of the buffer in bytes.
 *  @param usage Usage type of the buffer, i.e. VK_BUFFER_USAGE_VERTEX_BUFFER_BIT for VBO.
 *  @param properties Memory properties to request, i.e. VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT for VBO.
 *  @param physicalDevice The Vulkan physical device.
 *  @param device The Vulkan logical device.
 *  @param buffer Empty variable for buffer.
 *  @param bufferMemory Empty variable for buffer memory.
 */
void inline createBuffer(
    VkDeviceSize            size,
    VkBufferUsageFlags      usage,
    VkMemoryPropertyFlags   properties,
    VkPhysicalDevice        physicalDevice,
    VkDevice                device,
    VkBuffer                & buffer,
    VkDeviceMemory          & bufferMemory
) {
    // Create buffer object
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // will ONLY be used from graphics queue
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::CREATE_BUFFER::CREATION_FAILED");

    // Set up memory requirements and alloc info
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);

    /*
        Allocating memory for and every buffer if not good!
        The maxMemoryAllocationCount is very low.

        Use something like this instead in the future: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.
        Even better, follow these guidelines: https://developer.nvidia.com/vulkan-memory-management
        (In short, VBOs and EBO/IBO should be allocated into the SAME buffer. This allows the driver to re-use chunks of memory! AKA "aliasing".)
    */
    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::CREATE_BUFFER::ALLOCATION_FAILED");

    // Assign newly allocated memory to vbo
    // (for future reference: if last parameter is not 0, memory size must be aligned with memRequirements.alignments)
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

/**
 *  Copies the contents of a buffer onto another.
 *  Note: VK_BUFFER_USAGE_TRANSFER_SRC_BIT and VK_BUFFER_USAGE_TRANSFER_DST_BIT are required for the source and recipient, respectively.
 * 
 *  @param srcBuffer The buffer to copy from.
 *  @param dstBuffer The buffer to copy to.
 *  @param size How many bytes of the source buffer to copy (should be its size in most cases).
 *  @param device The Vulkan logical device.
 *  @param commandPool The command pool to use.
 *  @param queue The queue to copy-command with.
 */
void inline copyBuffer(
    VkBuffer        srcBuffer,
    VkBuffer        & dstBuffer,
    VkDeviceSize    size,
    VkDevice        device,
    VkCommandPool   commandPool,
    VkQueue         queue
) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer, commandPool, device, queue);
}

/**
 *  Creates a uniform buffer object.
 *  
 *  @param physicalDevice The Vulkan physical device.
 *  @param device The Vulkan logical device.
 *  @param buffers Empty vector for buffers.
 *  @param buffersMemory Empty vector for the buffers' memory.
 *  @param buffersMapped Empty vector for host visible buffer memory.
 */
template <typename T>
void inline createUniformBuffers(
    VkPhysicalDevice            physicalDevice,
    VkDevice                    device,
    std::vector<VkBuffer>       & buffers,
    std::vector<VkDeviceMemory> & buffersMemory,
    std::vector<void*>          & buffersMapped
) {
    VkDeviceSize bufferSize = sizeof(T);

    // Create space for buffers
    buffers.resize(MAX_FRAMES_IN_FLIGHT);
    buffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    buffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    // Create and map buffers
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            physicalDevice, device,
            buffers[i], buffersMemory[i]
        );

        vkMapMemory(device, buffersMemory[i], 0, bufferSize, 0, &buffersMapped[i]);
    }
}

/**
 *  Creates a set of SSBOs with the given data.
 */
template <typename T>
void inline createSSBO(
    VkPhysicalDevice            physicalDevice,
    VkDevice                    device,
    VkCommandPool               commandPool,
    VkQueue                     queue,
    std::vector<T>              data,
    std::vector<VkBuffer>       & buffer,
    std::vector<VkDeviceMemory> & bufferMemory
) {
    VkDeviceSize bufferSize = sizeof(T) * data.size();

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        physicalDevice, device,
        stagingBuffer, stagingBufferMemory
    );

    // Copy data to staging buffer
    void* ptr;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &ptr);
    memcpy(ptr, data.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    // Create SSBOs and copy staging buffer to them
    buffer.resize(MAX_FRAMES_IN_FLIGHT);
    bufferMemory.resize(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(
            bufferSize, 
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            physicalDevice, device,
            buffer[i], bufferMemory[i]
        );
        copyBuffer(
            stagingBuffer, buffer[i], bufferSize,
            device, commandPool, queue );
    }

    // Cleanup
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}