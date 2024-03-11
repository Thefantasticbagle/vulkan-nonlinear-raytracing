#include "vulkanApplication.h"

/**
 *  Locates memory of a given type and props.
 */
uint32_t VulkanApplication::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
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
        if (
            typeFilter & (1 << i) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties
            ) return i;
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

/**
 *  Creates a buffer.
 *
 *  @param size The size of the buffer in bytes.
 *  @param usage Usage type of the buffer, i.e. VK_BUFFER_USAGE_VERTEX_BUFFER_BIT for VBO.
 *  @param properties Memory properties to request, i.e. VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT for VBO.
 */
void VulkanApplication::createBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory
) {
    // Create buffer object
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // will ONLY be used from graphics queue
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        throw std::runtime_error("failed to create vertex buffer!");

    // Set up memory requirements && alloc info
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    /*
        Allocating memory for and every buffer if not good!
        The maxMemoryAllocationCount is very low.

        Use something like this instead in the future: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.
        Even better, follow these guidelines: https://developer.nvidia.com/vulkan-memory-management
        (In short, VBOs and EBO/IBO should be allocated into the SAME buffer. This allows the driver to re-use chunks of memory! AKA "aliasing".)
    */
    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate vertex buffer memory!");

    // Assign newly allocated memory to vbo
    // (for future reference: if last parameter is not 0, memory size must be aligned with memRequirements.alignments)
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

/**
 *  Copies the contents of a buffer onto another.
 *  Note: VK_BUFFER_USAGE_TRANSFER_SRC_BIT and VK_BUFFER_USAGE_TRANSFER_DST_BIT are required for the source and recipient, respectively.
 */
void VulkanApplication::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}