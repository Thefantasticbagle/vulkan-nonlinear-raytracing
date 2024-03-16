#include "vulkanApplication.h"

/**
  *  Creates the view for an image with a given format.
  */
VkImageView VulkanApplication::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
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
        throw std::runtime_error("failed to create image views!");
    return imageView;
}
