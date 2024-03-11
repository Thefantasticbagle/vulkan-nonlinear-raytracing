#include "vulkanApplication.h"

#include <iostream>

/**
 *	Initializes glfw and creates a window.
 */
void VulkanApplication::initWindow() {
	// Init glfw and tell it to NOT create an OpenGL context
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// Initialize window and add pointer to the member which created it
	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan-Compute", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

/**
 *  GLFW Callback for handling window resizing.
 *  (Needs to be static so it does not depend on this specific class member)
 */
void VulkanApplication::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    // Since this function is static we need to do some tricks to retrieve the class member it is called for
    // Earlier (see initWindow) we used "glfwSetWindowUserPointer(window, this)" to store a pointer to the class member in the window
    // We access this using the following command:
    auto app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));

    // signal to recreate swapchain and stuff
    app->framebufferResized = true;
}

/**
 *  Creates a surface object for the window.
 */
void VulkanApplication::createSurface() {
    // Use GLFW to create surface (This is less platform specific and very simple, which is the point of GLFW)
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::CREATE_SURFACE::CREATION_FAILED");
}