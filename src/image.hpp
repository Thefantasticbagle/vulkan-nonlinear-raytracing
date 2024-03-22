#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "buffer.hpp"

#include <stdexcept>


/**
 *  Simply checks if the given format contains a stencil component.
 */
bool inline hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

/**
 *  Creates an image object and allocates memory for an image.
 * 
 *  @param width The width of the image.
 *  @param height The height of the image.
 *  @param mipLevels The number of mipmap levels.
 *  @param format The image format.
 *  @param tiling Tiling mode.
 *  @param usage Image usage.
 *  @param properties Image properties.
 *  @param physicalDevice The Vulkan physical device.
 *  @param device The Vulkan logical device.
 *  @param image An empty variable for the image.
 *  @param imageMemory An empty variable for the image memory.
 */
void inline createImage(
    uint32_t                width,
    uint32_t                height,
    uint32_t                mipLevels,
    VkFormat                format,
    VkImageTiling           tiling,
    VkImageUsageFlags       usage,
    VkMemoryPropertyFlags   properties,
    VkPhysicalDevice        physicalDevice,
    VkDevice                device,
    VkImage                 & image,
    VkDeviceMemory          & imageMemory
) {
    // Create Vulkan image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling; // VK_IMAGE_TILING_LINEAR => row major, necessary for direct access of pixels
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Only necessary if the image is going to be a transfer source to another image
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT; // No multisampling
    imageInfo.flags = 0; // Flags exist for i.e. sparse images such as voxels with a lot of air

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
        throw std::runtime_error("failed to create image!");

    // Allocate memory
    // (Same as with UBO, but with vkGetImageMemoryRequirements and vkBindImageMemory instead of vkGetBufferMemoryRequirements and vkBindBufferMemory.)
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::CREATE_IMAGE::MEMORY_ALLOC_FAILURE");

    vkBindImageMemory(device, image, imageMemory, 0);
}

/**
 *  Creates the view for an image with a given format.
 * 
 *  @param image The image to create a view for.
 *  @param format Format of the image.
 *  @param aspectFlags Which aspects are included in the view.
 *  @param mipLevels How many mipmap levels the image has.
 *  @param device The Vulkan logical device.
 * 
 *  @return The image view.
 */
VkImageView inline createImageView(
    VkImage             image,
    VkFormat            format,
    VkImageAspectFlags  aspectFlags,
    uint32_t            mipLevels,
    VkDevice            device
) {
    // Create image view struct
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;

    // Other types include: 1D and 3D textures, cubemaps
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;

    // Use default mapping explicitly
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // Describe purpose of image and which parts should be accessed
    // (For i.e. stereoscopic 3D, use multiple layers and different views for each eye)
    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0; // No mipmapping
    createInfo.subresourceRange.levelCount = mipLevels;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    // Create image view and return
    VkImageView imageView;
    if (vkCreateImageView(device, &createInfo, nullptr, &imageView) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::CREATE_IMAGE_VIEW::CREATION_FAILED");
    return imageView;
}

/**
 *  Transitions an image from an old layout to a new one.
 *  Uses pre-programmed combinations.
 * 
 *  @param image The image to transition.
 *  @param format The image format.
 *  @param oldLayout The original image layout.
 *  @param newLayout Which layout to transition to.
 *  @param mipLevels the number of mipmap levels.
 *  @param device The Vulkan logical device.
 *  @param commandPool Command pool.
 *  @param queue Queue.
 */
void inline transitionImageLayout (
    VkImage         image,
    VkFormat        format,
    VkImageLayout   oldLayout,
    VkImageLayout   newLayout,
    uint32_t        mipLevels,
    VkDevice        device,
    VkCommandPool   commandPool,
    VkQueue         queue
) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    // Set up barrier
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    // (We are not using the barrier to transfer family ownership)
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;

    //aspectMask
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    // Depending on oldformat and newformat, different parts of the program should have to wait
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    /*
        https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap7.html#VkPipelineStageFlagBits
        ^ Information about all possible stages, including psuedo-stages.
    */

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        // For Undefined -> Transfer destination: Transfer writes wait for nothing
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        // For Transfer destination -> Sampling: Fragment shader reads wait for Transfer writes
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        // For Undefined -> Depth stencil attachment (optimal): ?
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else {
        // Undefined interaction
        throw std::invalid_argument("ERR::VULKAN::TRANSITION_IMAGE_LAYOUT::TRANSITION_NOT_PROGRAMMED");
    }

    // Engage barrier
    // See https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap7.html#synchronization-access-types-supported for 2. param
    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer, commandPool, device, queue); // TODO: ADD A WAY TO CHANGE QUEUE
}