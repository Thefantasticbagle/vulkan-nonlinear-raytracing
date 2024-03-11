#include "vulkanApplication.h"

#include <iostream>

/**
 *  Checks which validation layers are available.
 */
bool VulkanApplication::checkValidationLayerSupport() {
    // Get amount of layers
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    // Fetch available layers
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // Iterate enabled validation layers, checking if each is available
    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

/**
 *  Gets the required extensions for the set validation layers.
 */
std::vector<const char*> VulkanApplication::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

/**
 *  Populates a VkDebugUtilsMessengerCreateInfoEXT object.
 */
void VulkanApplication::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    // Types of severities to receive callback for, all except VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT is included
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    // Filter by message type
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    // Pointer to callback function
    createInfo.pfnUserCallback = debugCallback;
}

/**
 *  Debug callback function.
 *
 *  @param messageSeverity
 *      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT - Diagnostic message.
 *      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    - Informational messasge.
 *      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT - Likely bug.
 *      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT   - Crashing/invalid code.
 *      These enumerations can be compared in degrees of severity (A > B).
 *  @param messageType
 *      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     - Event unrelated to spec and performance.
 *      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  - Spec violated :(
 *      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT - Potential non-optimal use of Vulkan.
 *  @param pCallbackData struct containing details, important fields are as follows:
 *      .pMessage - Debug message as null terminated string.
 *      .pObjects - Array of Vulkan objects related to message.
 *      .objectCount - Length of array.
 *  @param pUserData Pointer which data can be passed to.
 *  @return Whether to abort or not.
 */
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanApplication::debugCallback (
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

/**
 *  Sets up debug messenger for validation layers.
 *  This does NOT debug instance creation.
 */
void VulkanApplication::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    // Create message struct
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    // Create debug messenger object
    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::SETUP_DEBUG_MESSENGER::CREATION_FAILED");
}