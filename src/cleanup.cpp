#include "vulkanApplication.h"

void VulkanApplication::cleanup() {
    // Cleanup vulkan
    //swapchain
    cleanupSwapChain();

    //buffers & images
    vkDestroyImageView(device, textureImageView, nullptr);
    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);

        vkDestroyBuffer(device, shaderStorageBuffers[i], nullptr);
        vkFreeMemory(device, shaderStorageBuffersMemory[i], nullptr);
    }
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    //pipeline
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

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