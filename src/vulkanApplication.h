#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES // Use alignas(16) instead
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "raytracing.hpp"
#include "buffer.hpp"
#include "command.hpp"
#include "image.hpp"
#include "texture.hpp"

#include <vector>
#include <optional>
#include <fstream>
#include <array>

// Constants
const uint32_t  WIDTH = 800;
const uint32_t  HEIGHT = 600;
const int       MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const bool enableValidationLayers = true;
#ifdef NDEBUG
    enableValidationLayers = false;
#endif

// Functions loaded with ProcAddr
// vkCreateDebugUtilsMessengerEXT
static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, 
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
    const VkAllocationCallbacks* pAllocator, 
    VkDebugUtilsMessengerEXT* pDebugMessenger) {

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

// vkDestroyDebugUtilsMessengerEXT
static void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator) {

    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

// Utility

/**
 *  Reads the contents of a file.
 */
static std::vector<char> readFile(const std::string& filename) {
    // Start reading at the end of the file to determine size
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("ERR::READ_FILE::FAILURE_OPENING_FILE");

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    // Go back to beginning of file and read contents in order
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    // Cleanup and return
    file.close();
    return buffer;
}

// Structs
/**
 *  Struct for holdind queue families.
 */
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsAndComputeFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
    }
};

/**
 *  Struct for storing details about swapchain support.
 */
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;      //numbers of imgs, dimensions, etc
    std::vector<VkSurfaceFormatKHR> formats;    //pixel format/color space 
    std::vector<VkPresentModeKHR> presentModes; //available presentation modes
};

/**
 *	The vulkan application class.
 */
class VulkanApplication {
public:
    void run() {
        // Set up vulkan context
        //listExtensions();
        initWindow();
        
        createInstance();
        setupDebugMessenger();

        createSurface();

        pickPhysicalDevice();
        createLogicalDevice();

        createSwapChain();
        createImageViews();
        createRenderPass();
        createFramebuffers();

        createCommandPool(
            physicalDevice,
            device,
            findQueueFamilies(physicalDevice).graphicsAndComputeFamily.value(),
            commandPool );

        // Create texture/image for framebuffer to sample from
        createTextureImage(
            static_cast<uint32_t>(swapChainExtent.width), static_cast<uint32_t>(swapChainExtent.height),
            device, physicalDevice, commandPool, graphicsQueue,
            textureImage, textureImageMemory );
        createTextureImageView(
            textureImage,
            device,
            textureImageView );
        createTextureSampler(
            physicalDevice, device,
            textureSampler);

        // Set up RTSpheres
        std::vector<RTSphere> spheres {
            RTSphere {
                1.f,
                glm::vec3(0,0,2),
                RTMaterial {
                    glm::vec4(1,0,0,1),
                    glm::vec4(1,0,0,1),
                    glm::vec4(1,0,0,1),
                    1.f
                }
            }
        };

        createSSBO(
            physicalDevice, device,
            commandPool, computeQueue,
            spheres,
            shaderStorageBuffers, shaderStorageBuffersMemory );
        createUniformBuffers<RTParams>(
            physicalDevice, device,
            uniformBuffers, uniformBuffersMemory, uniformBuffersMapped);

        createComputeDescriptorPool();
        createFragmentDescriptorPool();

        createComputeDescriptorSetLayout();
        createFragmentDescriptorSetLayout();
        
        createComputeDescriptorSets();
        createFragmentDescriptorSets();

        createComputeCommandBuffers();
        createGraphicsCommandBuffers();

        createGraphicsPipeline();
        createComputePipeline();

        createSyncObjects();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();

            // Time
            double currentTime = glfwGetTime();
            lastFrameTime = (currentTime - lastTime) * 1000.0;
            lastTime = currentTime;
            totalTime += lastFrameTime;
        }

        vkDeviceWaitIdle(device);
        cleanup();
    }

private:
    // Window
    GLFWwindow* window;
    bool framebufferResized = false;
    VkSurfaceKHR surface;

    // Instance
    VkInstance instance;

    // Validation
    VkDebugUtilsMessengerEXT debugMessenger;

    // Device
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    // Queues
    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkQueue presentQueue;

    // Swapchain
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    // Global (Graphics + Compute)
    VkCommandPool    commandPool;

    // Texture
    VkImage         textureImage;
    VkDeviceMemory  textureImageMemory;
    VkImageView     textureImageView;
    VkSampler       textureSampler;

    // SSBO
    std::vector<VkBuffer>       shaderStorageBuffers;
    std::vector<VkDeviceMemory> shaderStorageBuffersMemory;

    // Uniform buffer
    std::vector<VkBuffer>       uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*>          uniformBuffersMapped;

    // Graphics
    VkRenderPass                    renderPass;
    VkPipelineLayout                graphicsPipelineLayout;
    VkPipeline                      graphicsPipeline;
    std::vector<VkCommandBuffer>    graphicsCommandBuffers;
    VkDescriptorPool                graphicsDescriptorPool;
    VkDescriptorSetLayout           fragmentDescriptorSetLayout;
    std::vector<VkDescriptorSet>    fragmentDescriptorSets;

    // Compute
    VkPipelineLayout                computePipelineLayout;
    VkPipeline                      computePipeline;
    std::vector<VkCommandBuffer>    computeCommandBuffers;
    VkDescriptorPool                computeDescriptorPool;
    VkDescriptorSetLayout           computeDescriptorSetLayout;
    std::vector<VkDescriptorSet>    computeDescriptorSets;

    // Synchronization
    std::vector<VkSemaphore>    imageAvailableSemaphores;
    std::vector<VkSemaphore>    renderFinishedSemaphores;
    std::vector<VkSemaphore>    computeFinishedSemaphores;
    std::vector<VkFence>        inFlightFences;
    std::vector<VkFence>        computeInFlightFences;

    // Drawing
    uint32_t currentFrame = 0;
    float lastFrameTime = 0.0f;
    double lastTime = 0.0f;
    float totalTime = 0.f;

    // Functions
    void initWindow();
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    void createSurface();

    void createInstance();

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
    void setupDebugMessenger();

    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    void createLogicalDevice();

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    void createSwapChain();
    void recreateSwapChain();
    void cleanupSwapChain();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createImageViews();
    void createFramebuffers();

    void createComputeDescriptorPool();
    void createFragmentDescriptorPool();
    void createComputeDescriptorSetLayout();
    void createFragmentDescriptorSetLayout();
    void createComputeDescriptorSets();
    void createFragmentDescriptorSets();

    void createRenderPass();
    void createGraphicsPipeline();
    void createComputePipeline();

    //void createGraphicsCommandBuffers();
    //void createComputeCommandBuffers();
    void recordGraphicsCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void recordComputeCommandBuffer(VkCommandBuffer commandBuffer);

    void createSyncObjects();
    void drawFrame();
    
    void cleanup();

    VkShaderModule createShaderModule(const std::vector<char>& code);

    /**
     *  Allocates a commandbuffer.
     *  TODO: MAKE FUNCTIONAL.
     */
    void createGraphicsCommandBuffers() {
        graphicsCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        // Create command buffer allocator
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        // VK_COMMAND_BUFFER_LEVEL_PRIMARY   - Can be submitted, cannot be called from other command buffers.
        // VK_COMMAND_BUFFER_LEVEL_SECONDARY - Cannot be submitted, can be called from primary buffers (good for reuse).
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)graphicsCommandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo, graphicsCommandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("ERR::VULKAN::CREATE_COMMAND_BUFFERS::ALLOCATION_FAILED");
    }

    /**
     *  Allocate a commandbuffer for compute.
     *  TODO: MAKE FUNCTIONAL.
     */
    void createComputeCommandBuffers() {
        computeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        // Create command buffer allocator
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        // VK_COMMAND_BUFFER_LEVEL_PRIMARY   - Can be submitted, cannot be called from other command buffers.
        // VK_COMMAND_BUFFER_LEVEL_SECONDARY - Cannot be submitted, can be called from primary buffers (good for reuse).
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)computeCommandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo, computeCommandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("ERR::VULKAN::CREATE_COMMAND_BUFFERS::ALLOCATION_FAILED");
    }

    /**
     *  Updates the contents of the UBO for the given in-flight frame.
     *  TODO: MAKE FUNCTIONAL
     */
    void VulkanApplication::updateUniformBuffer(uint32_t currentImage) {
        RTCamera camera = RTCamera(
            glm::zero<glm::vec3>(),
            glm::zero<glm::vec3>(),
            glm::vec2(static_cast<uint32_t>(swapChainExtent.width), static_cast<uint32_t>(swapChainExtent.height)),
            1.f,
            60.f,
            1.f,
            10.f
        );

        // Set up contents of UBO
        RTParams ubo{};
        ubo.screenSize = camera.screenSize;
        ubo.fov = camera.fov;
        ubo.focusDistance = camera.focusDistance;
        ubo.cameraPos = camera.pos;
        ubo.localToWorld = camera.rts;
        ubo.frameNumber = 2;
        
        ubo.maxBounces = 10;
        ubo.raysPerFrag = 3;
        ubo.divergeStrength = 0.01f;
        ubo.blackholePower = 1.f;
        
        ubo.spheresCount = 1;
        ubo.blackholesCount = 1;
        ubo.deltaTime = lastFrameTime * 2.f;

        // Copy contents into buffer
        // (This is less efficient than using "push constants"
        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }
};