#include "vulkanApplication.h"

#include <iostream>
#include <set>

/**
 *	Picks a physical device.
 */
void VulkanApplication::pickPhysicalDevice() {
    // Get amount of physical devices available
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    // If there are no availabe devices, throw error
    if (deviceCount == 0) throw std::runtime_error("ERR::VULKAN::PICK_PHYSICAL_DEVICE::NO_VULKAN_GPU");

    // Fetch all physical devices
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // Iterate them, checking if any are suitable
    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    // If not, throw error
    if (physicalDevice == VK_NULL_HANDLE) throw std::runtime_error("ERR::VULKAN::PICK_PHYSICAL_DEVICE::NO_SUITABLE_GPU");
}

/**
 *  Whether a physical device is suitable for the operations we want to perform.
 *  TODO: Add score system instead of picking first viable GPU.
 */
bool VulkanApplication::isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    // Check for extension support
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    // Check for swapchain support
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    // This should be put in some sort of vector at the top of the code, also, update the VkPhysicalDeviceFeatures struct.
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
    bool isValid = indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeatures{};
    accelFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    VkPhysicalDeviceFeatures2 supportedFeatures2;
    supportedFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    supportedFeatures2.pNext = &accelFeatures;
    vkGetPhysicalDeviceFeatures2(device, &supportedFeatures2);
    isValid &= accelFeatures.accelerationStructure;

    // (Also, for now, require that a DEDICATED GPU is used)
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);
    bool isDedicated = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

    printf("GPU with name ");
    printf(props.deviceName);
    if (isDedicated) printf(" is DEDICATED");
    else printf(" is INTEGRATED");
    if (isValid) printf(" and VALID.\n");
    else printf(" and INVALID.\n");

    return isValid && isDedicated;
}

/**
 *  Iterates available queue families, selecting a suitable one before returning it.
 */
QueueFamilyIndices VulkanApplication::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    // Get amount of available queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    // Get queue families
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    // Iterate queue families, checking their validities
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        //check if the current family supports rendering to khr surface
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport)
            indices.presentFamily = i;

        //check if family supports VK_QUEUE_GRAPHICS_BIT and COMPUTE_BIT
        if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
            indices.graphicsAndComputeFamily = i;

        // (Early exit)
        if (indices.isComplete()) break;
        i++;
    }

    return indices;
}

/**
 *  Whether a physical device supports the set extensions.
 */
bool VulkanApplication::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    // Get extensions
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
     
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    // Create set of requested extensions and remove them iteratively if supported
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtensions)
        requiredExtensions.erase(extension.extensionName);

    // Return
    return requiredExtensions.empty();
}

/**
 *  Creates a logical device using the set physicalDevice.
 */
void VulkanApplication::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    // Create queue info struct
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsAndComputeFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // (Later I might want to add a feature like VK_KHR_swapchain)
    //VkPhysicalDeviceFeatures deviceFeatures{};
    //deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeatures{};
    accelFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accelFeatures.accelerationStructure = VK_TRUE;

    VkPhysicalDeviceBufferDeviceAddressFeatures deviceFeatures{};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR;
    deviceFeatures.bufferDeviceAddress = VK_TRUE;
    deviceFeatures.pNext = &accelFeatures;

    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &deviceFeatures;
    deviceFeatures2.features.samplerAnisotropy = VK_TRUE;

    // Create device info struct
    // TODO: ADD VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR AND MORE TO PNEXT
    // (Even if it works without, they should probably be here...)
    // https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pNext = &deviceFeatures2;

    // Enable extensions explicitly
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // Older versions may differentiate instance- and device specific layers
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    // Create logical device
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::CREATE_LOGICAL_DEVICE::CREATION_FAILED");

    // Create handle to device queue
    // (These will most likely have the same values, unless one device is not able to i.e. render)
    vkGetDeviceQueue(device, indices.graphicsAndComputeFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.graphicsAndComputeFamily.value(), 0, &computeQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}