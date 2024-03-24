#include "vulkanApplication.h"

#include <stdexcept>

/**
 *	Creates a Vulkan instance.
 */
void VulkanApplication::createInstance() {
	// Check if validation layers are available/set up correctly
	if (enableValidationLayers && !checkValidationLayerSupport())
		throw std::runtime_error("ERR::VULKAN::CREATE_INSTANCE::VALIDATION_LAYERS_UNAVAILABLE");

    // Set up app info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan-Compute";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 3, 0);//VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3; //VK_API_VERSION_1_0;

    // Tell Vulkan driver which global extensions/validation layer to use
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Get platform specific extension details (GLFW makes this simple)
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Enable validation layers
    //create debug object here so that it is picked up by vkCreateInstance later
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        // Check for errors
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);

    }
    else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    // Create instance and error handle
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS) throw std::runtime_error("ERR::VULKAN::CREATE_INSTANCE::CREATION_FAILED");
}