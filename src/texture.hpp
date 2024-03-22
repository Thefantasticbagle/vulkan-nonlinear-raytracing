#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "image.hpp"

/**
 *  Reads, allocates, and creates an example texture.
 */
void inline createTextureImage(
    uint32_t            width,
    uint32_t            height,
    VkDevice            device,
    VkPhysicalDevice    physicalDevice,
    VkCommandPool       commandPool,
    VkQueue             queue,
    VkImage             & textureImage,
    VkDeviceMemory      & textureImageMemory
) {
    // Check if image format supports linear blit
    /*VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        throw std::runtime_error("texture image format does not support linear blitting!");*/

    // Create and allocate image
    createImage(
        width, height, 1,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        physicalDevice,
        device,
        textureImage,
        textureImageMemory
    );

    // Transition the receiving image's format to a copy-optimised one and copy the stagingbuffer to it
    // Note: The VK_IMAGE_LAYOUT_GENERAL format supports all operations and can be good for testing, but is not optimal.
    transitionImageLayout(
        textureImage,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL,//VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        1,
        device,
        commandPool,
        queue
    );
}

/**
 *  Creates the view for the texture.
 */
void inline createTextureImageView(
    VkImage     textureImage,
    VkDevice    device,
    VkImageView & textureImageView
) {
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1, device);
}

/**
 *  Creates the sampler for the texture.
 */
void inline createTextureSampler(
    VkPhysicalDevice    physicalDevice,
    VkDevice            device,
    VkSampler           & textureSampler
) {
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
    samplerInfo.maxLod = 1;
    samplerInfo.mipLodBias = 0.0f; // Optional

    //set up anisotropic filtering
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; // Color of outside of texture dimensions
    samplerInfo.unnormalizedCoordinates = VK_FALSE; // True -> [0,width) and [0,height) instead of [0,1)

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::CREATE_TEXTURE_SAMPLER::CREATION_FAILED");
}