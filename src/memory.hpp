#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "command.hpp"

#include <stdexcept>
#include <vector>


/**
 *  Class for creating a buffer object, i.e. UBO or SSBO.
 *  Does NOT handle the deletion of its buffers and buffer memories.
 */
class Buffer {
public:
    // Exposed fields
    std::vector<VkBuffer>       buffers;
    std::vector<VkDeviceMemory> buffersMemory;
    std::vector<void*>          buffersMapped;

    /**
     *  Complete constructor.
     *  Allocates memory for the buffer and its fields, which are stored in the exposed fields.
     * 
     *  @param mainUsage The main usage of the buffer, i.e. VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT for Uniform Buffer Objects.
     *  @param isMapped Whether the buffer should be consistently mapped and host visible.
     *  @param initialData The initial data of the buffer, if any. For UBOs, the vector's first element is used.
     *  @param physicalDevice The Vulkan physical device.
     *  @param device The Vulkan logical device.
     *  @param commandPool A command pool to pull commands from.
     *  @param queue The queue to commit commands to.
     */
    template<typename T>
    Buffer (
        VkBufferUsageFlags  mainUsage,
        bool                isMapped,
        std::vector<T>      initialData,
        VkPhysicalDevice    physicalDevice,
        VkDevice            device,
        VkCommandPool       commandPool,
        VkQueue             queue
    ) {
        VkMemoryPropertyFlags   props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VkBufferUsageFlags      usage = mainUsage;
        VkBuffer                stagingBuffer;
        VkDeviceMemory          stagingBufferMemory;
        VkDeviceSize            bufferSize = sizeof(T);

        // If the buffer is mapped, make it host visible and coherent
        if (isMapped) {
            props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            buffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
        }
        // If the buffer is not mapped and has initial data, stage it
        else if (initialData.size() > 0) {
            bufferSize *= initialData.size();
            usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            
            // Create staging buffer
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
            memcpy(ptr, initialData.data(), (size_t)bufferSize);
            vkUnmapMemory(device, stagingBufferMemory);
        }

        // Create main buffers (and transfer data)
        buffers.resize(MAX_FRAMES_IN_FLIGHT);
        buffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer( bufferSize, usage, props, physicalDevice, device, buffers[i], buffersMemory[i] );

            if (isMapped)
                vkMapMemory(device, buffersMemory[i], 0, bufferSize, 0, &buffersMapped[i]);

            if (isMapped && initialData.size() > 0)
                memcpy(buffersMapped[i], &initialData[0], sizeof(T));

            if (!isMapped && initialData.size() > 0)
                copyBuffer( stagingBuffer, buffers[i], bufferSize, device, commandPool, queue );
        }

        // Cleanup
        if (!isMapped && initialData.size() > 0) {
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
        }
    }
};

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
 *  Copies a buffer to an image.
 *
 *  @param buffer The buffer to copy from.
 *  @param image The image to copy to.
 *  @param width Width of the image region to copy to.
 *  @param height Height of the image region to copy to.
 *  @param device The Vulkan logical device.
 *  @param commandPool The command pool to use.
 *  @param queue The queue to copy-command with.
 */
void inline copyBufferToImage(
    VkBuffer        buffer,
    VkImage         & image,
    uint32_t        width,
    uint32_t        height,
    VkDevice        device,
    VkCommandPool   commandPool,
    VkQueue         queue
) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    // Define which regions map to where
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    // Copy buffer to image
    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Assumes that the image has already been transitioned to the optimal pixel-copying format
        1,
        &region
    );

    endSingleTimeCommands(commandBuffer, commandPool, device, queue);
}