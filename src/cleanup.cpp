#include "vulkanApplication.h"

void VulkanApplication::cleanup() {
    // Cleanup vulkan
    //swapchain
    cleanupSwapChain();

    //pipeline
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr);

    vkDestroyPipeline(device, computePipeline, nullptr);
    vkDestroyPipelineLayout(device, computePipelineLayout, nullptr);

    //renderpass
    vkDestroyRenderPass(device, renderPass, nullptr);

    //synchronization objects
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
        vkDestroyFence(device, computeInFlightFences[i], nullptr);
        vkDestroySemaphore(device, computeFinishedSemaphores[i], nullptr);
    }

    //commandpool
    vkDestroyCommandPool(device, commandPool, nullptr);

    //logical device
    vkDestroyDevice(device, nullptr);

    //debug messenger
    if (enableValidationLayers) DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

    //surface and instance
    vkDestroySurfaceKHR(instance, surface, nullptr); // Platform specific, but GLFW does not offer call to delete surface
    vkDestroyInstance(instance, nullptr);

    // Cleanup GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
}