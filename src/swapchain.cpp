#include "vulkanApplication.h"

#include <algorithm> // clamp
#include <stdexcept>

/**
 *  Populates the SwapChainSupportDetails struct.
 */
SwapChainSupportDetails VulkanApplication::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    // Query basic surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // Query supported surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    // Query supported presentation modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

/**
 *  Creates a swapchain.
 */
void VulkanApplication::createSwapChain() {
    // Create swapchain support struct
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    // Set optimal surface format/present mode/extent
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    // Ask for an additional image to avoid having to wait for driver's internal ops
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;

    // Create swapchain info struct
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1; // 1 unless making i.e. a stereoscopic 3D app
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // If post-processing, VK_IMAGE_USAGE_TRANSFER_DST_BIT

    // Set vulkan sharing mode based on graphics- and presentation family
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Requires 2+ distinct queue families
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // Best performance
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    // Tell Vulkan we dont want any special transformation
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

    // Window should be opaque
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE; // Ignore pixels obscured by other windows

    // If i.e. the window is resized, the swapchain must be remade from scratch using the previous one as basis
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    // Create swapchain object
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
        throw std::runtime_error("failed to create swap chain!");

    // Fetch handles for images
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    // Store swapchain information for later use
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

/**
  *  Recreates the swap chain for when window is resized and such.
  *  This temporarily freezes operations, which can be avoided by making a new swapchain from the old one WHILE its rendering.
  *  (Also, this does not recreate the renderpass, but this is only necessary if we are changing from i.e. HDR screen to normal)
  */
void VulkanApplication::recreateSwapChain() {
    // Check if the window was minimized (framebuffer size 0), if so, wait for it to be maximized
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    // Wait until GPU is done with processing current frames
    vkDeviceWaitIdle(device);

    // Clean up the previous- and create the new swapChain, imageViews, and frameBuffers
    cleanupSwapChain();
    createSwapChain();
    createImageViews();
    createDepthResources();
    createFramebuffers();
}

/**
 *  Cleans up the swapchain, imageviews, and framebuffers.
 */
void VulkanApplication::cleanupSwapChain() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    for (auto framebuffer : swapChainFramebuffers) vkDestroyFramebuffer(device, framebuffer, nullptr);
    for (auto imageView : swapChainImageViews) vkDestroyImageView(device, imageView, nullptr);
    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

/**
 *  Selects the best surface format from a list.
 *  For now, prefers SRGB, selects whatever otherwise.
 */
VkSurfaceFormatKHR VulkanApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // Look for SRGB format
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    // If it was not found, choose whatever
    return availableFormats[0];
}

/**
 *  Selects the best presentation mode from a list.
 *  VK_PRESENT_MODE_FIFO_KHR is the only one guaranteed to be available.
 *  The four possible modes in Vulkan are:
 *      VK_PRESENT_MODE_IMMEDIATE_KHR       - Straight to screen, causes tearing.
 *      VK_PRESENT_MODE_FIFO_KHR            - Take from front of queue, program pauses if queue is full. "vsync". Good battery usage.
 *      VK_PRESENT_MODE_FIFO_RELAXED_KHR    - Same as FIFO_KHR, but uses IMMEDIATE_KHR when queue is empty.
 *      VK_PRESENT_MODE_MAILBOX_KHR         - Same as FIFO_KHR, but overwrites queue when full. "Tripple buffering". Good tradeoff.
 */
VkPresentModeKHR VulkanApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // Look for VK_PRESENT_MODE_MAILBOX_KHR
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    // If not available, use FIFO_KHR.
    return VK_PRESENT_MODE_FIFO_KHR;
}

/**
 *  Selects the best capabilities.
 */
VkExtent2D VulkanApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    // If width is defined, return the current extent
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    // Otherwise, use GLFW to get extent
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

/**
 *  Creates image views for all images in the swapchain.
 */
void VulkanApplication::createImageViews() {
    // Iterate swapchain images
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++)
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

/**
 *  Creates framebuffer objects for each imageview in the swapchain.
 */
void VulkanApplication::createFramebuffers() {
    // Iterate image views, creating a framebuffer for each
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            swapChainImageViews[i],
            depthImageView // Since only a single subpass is running at any time duing to our semaphores, we just need one depth image
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("ERR::VULKAN::CREATE_FRAME_BUFFERS::CREATION_FAILED");
    }
}