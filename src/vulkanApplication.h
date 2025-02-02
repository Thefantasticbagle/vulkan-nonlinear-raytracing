#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES // Use alignas(16) instead
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VulkanApplicationSettings.h"

#include "camera.hpp"
#include "glsl_cpp_common.h"
#include "buffer.hpp"

#include <vector>
#include <optional>
#include <fstream>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
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

        // Set up RTSpheres
        std::vector<RTSphere> spheres {
            //RTSphere {
            //    1.f,
            //    glm::vec3(0,0,14),
            //    RTMaterial {
            //        glm::vec4(1,1,1,1),
            //        glm::vec4(1,1,1,0),
            //        glm::vec4(1,1,1,0.95f),
            //        1.f
            //    }
            //},
            //RTSphere {
            //    10.f,
            //    glm::vec3(0,5,-16),
            //    RTMaterial {
            //        glm::vec4(1,0.3,0,1),
            //        glm::vec4(1,0.3,0,0.5f),
            //        glm::vec4(1,1,1,0.0f),
            //        0.5f
            //    }
            //},
            //RTSphere {
            //    100.f,
            //    glm::vec3(0,-100,0),
            //    RTMaterial {
            //        glm::vec4(1,1,1,1),
            //        glm::vec4(0,1,0,0.f),
            //        glm::vec4(0,1,0,0.f),
            //        0.f
            //    }
            //}
        };

        // Set up RTBlackholes
        std::vector<RTBlackhole> blackholes {
            RTBlackhole {
                0.5f,
                glm::vec3(0,1,6),
            }
        };

        std::vector<RTTorus> torus {
            RTTorus {
                glm::vec4(0.f, 1.f, 6.f, 3.5f / 1.25f),
                glm::vec4(3.14f / 2.f, 0.f, 0.f, 0.05f / 1.25f),
                    RTMaterial{
                        glm::vec4(1.f,0.7f,0.3f,0.1f),
                        glm::vec4(1.f,0.5f,0.1f,0.8f),
                        glm::vec4(1.f,0.7f,0.3f,0.0f),
                        0.5f
                }
            },
            RTTorus {
                glm::vec4(0.f, 1.f, 6.f, 3.4f / 1.25f),
                glm::vec4(3.14f / 2.f, 0.f, 0.f, 0.1f / 1.25f),
                    RTMaterial{
                        glm::vec4(1.f,0.4f,0.3f,0.1f),
                        glm::vec4(1.f,0.5f,0.1f,0.8f),
                        glm::vec4(1.f,0.7f,0.3f,0.0f),
                        0.5f
                }
            },
            RTTorus {
                glm::vec4(0.f, 1.f, 6.f, 3.1f / 1.25f),
                glm::vec4(3.14f / 2.f, 0.f, 0.f, 0.1f / 1.25f),
                    RTMaterial{
                        glm::vec4(1.f,0.4f,0.3f,0.1f),
                        glm::vec4(1.0f,0.7f,1.0f,0.8f),
                        glm::vec4(1.f,0.7f,0.3f,0.0f),
                        0.5f
                }
            },
            RTTorus {
                glm::vec4(0.f, 1.f, 6.f, 2.7f / 1.25f),
                glm::vec4(3.14f / 2.f, 0.f, 0.f, 0.15f / 1.25f),
                    RTMaterial{
                        glm::vec4(1.f,0.4f,0.3f,0.1f),
                        glm::vec4(1.0f,0.7f,1.0f,2.f),
                        glm::vec4(1.f,0.7f,0.3f,0.0f),
                        0.5f
                }
            },


            //RTTorus {
            //    glm::vec4(0.f, 1.f, 6.f, 2.3f / 1.25f),
            //    glm::vec4(3.14f / 2.f, 0.f, 0.f, 0.2f / 1.25f),
            //        RTMaterial{
            //            glm::vec4(1.f,0.4f,0.3f,0.1f),
            //            glm::vec4(1.f,0.5f,0.1f,1.f),
            //            glm::vec4(1.f,0.7f,0.3f,0.0f),
            //            0.5f
            //    }
            //},
            //RTTorus {
            //    glm::vec4(0.f, 1.f, 6.f, 2.0f / 1.25f),
            //    glm::vec4(3.14f / 2.f, 0.f, 0.f, 0.2f / 1.25f),
            //        RTMaterial{
            //            glm::vec4(1.f,0.4f,0.3f,0.1f),
            //            glm::vec4(1.0f,0.7f,1.0f,2.f),
            //            glm::vec4(1.f,0.7f,0.3f,0.0f),
            //            0.5f
            //    }
            //},
            //RTTorus {
            //    glm::vec4(0.f, 1.f, 6.f, 1.8f / 1.25f),
            //    glm::vec4(3.14f / 2.f, 0.f, 0.f, 0.25f / 1.25f),
            //        RTMaterial{
            //            glm::vec4(1.f,0.4f,0.3f,0.1f),
            //            glm::vec4(1.0f,0.7f,1.0f,2.5f),
            //            glm::vec4(1.f,0.7f,0.3f,0.0f),
            //            0.5f
            //    }
            //},
            //RTTorus {
            //    glm::vec4(0.f, 1.f, 6.f, 0.5),
            //    glm::vec4(3.14f / 2.f, 0.f, 0.f, 0.05),
            //        RTMaterial{
            //            glm::vec4(1.f,0.4f,0.3f,0.1f),
            //            glm::vec4(1.0f,0.7f,1.0f,10.f),
            //            glm::vec4(1.f,0.7f,0.3f,0.0f),
            //            0.5f
            //    }
            //},
        };

        // Make a buffer for holding dispatch size
        struct DispatchIndirectCommand {
            uint32_t x;
            uint32_t y;
            uint32_t z;
        } dispatchCmd = { WIDTH / 32, HEIGHT / 32, 1 }; // Example workgroup counts

        VkDeviceSize bufferSize = sizeof(DispatchIndirectCommand);
        dispatchBuffer = createBuffer(
            physicalDevice, device,
            bufferSize,
            VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            dispatchBufferMemory
        );

        // Upload the dispatch command to the buffer
        void* data;
        vkMapMemory(device, dispatchBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, &dispatchCmd, sizeof(dispatchCmd));
        vkUnmapMemory(device, dispatchBufferMemory);

        // Set up RTParams
        RTParams ubo{};
        ubo.screenSize = camera.screenSize;
        ubo.fov = camera.fov;
        ubo.focusDistance = camera.focusDistance;
        
        ubo.maxBounces = 1;
        ubo.raysPerFrag = 12;
        ubo.divergeStrength = 0.025f;
        ubo.blackholePower = 1.f;
        
        ubo.spheresCount = spheres.size();
        ubo.blackholesCount = blackholes.size();
        ubo.torusCount = torus.size();

        // Create buffers and layout
        computeBundle = BufferBuilder(physicalDevice, device, commandPool, computeQueue, &deletionQueue)
            .UBO(b_params, VK_SHADER_STAGE_COMPUTE_BIT, std::vector<RTParams>{ubo})
            .SSBO(b_spheres, VK_SHADER_STAGE_COMPUTE_BIT, spheres)
            .SSBO(b_blackholes, VK_SHADER_STAGE_COMPUTE_BIT, blackholes)
            .genericImage(b_image, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, true, true, nullptr, nullptr, swapChainExtent.width, swapChainExtent.height)
            .sampler(b_skybox, VK_SHADER_STAGE_COMPUTE_BIT, "../resources/textures/texture.jpg")
            .SSBO(b_torus, VK_SHADER_STAGE_COMPUTE_BIT, torus)
            .build();

        graphicsBundle = BufferBuilder(physicalDevice, device, commandPool, graphicsQueue, &deletionQueue)
            .sampler(0, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, &computeBundle.imageMemories[b_image])
            .build();

        computePushConstantReference = &frame;
        computePushConstantSize = sizeof(RTFrame);

        createComputeCommandBuffers();
        createGraphicsCommandBuffers();

        createGraphicsPipeline();
        createComputePipeline();

        createSyncObjects();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();

            float cameraRotationSpeed = 3.f;
            float cameraSpeed = 3.f;
            glm::vec3 dtAng = glm::zero<glm::vec3>();
            glm::vec3 dtPos = glm::zero<glm::vec3>();
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                dtAng.y += lastFrameTime * cameraRotationSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
                dtAng.y -= lastFrameTime * cameraRotationSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
                dtAng.x -= lastFrameTime * cameraRotationSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
                dtAng.x += lastFrameTime * cameraRotationSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
                printf("FPS = %i\n", (int)(1.f / lastFrameTime));
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                dtPos -= camera.left * lastFrameTime * cameraSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                dtPos += camera.left * lastFrameTime * cameraSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                dtPos += camera.front * lastFrameTime * cameraSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                dtPos -= camera.front * lastFrameTime * cameraSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                dtPos += camera.up * lastFrameTime * cameraSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                dtPos -= camera.up * lastFrameTime * cameraSpeed;
            }

            if (glm::length(dtAng) > 0.f)
                camera.ang += dtAng;
            if (glm::length(dtPos) > 0.f)
                camera.pos += dtPos;
            if (glm::length(dtAng) > 0.f || glm::length(dtPos) > 0.f)
                camera.calculateRTS();

            frame.cameraPos = camera.pos;
            frame.localToWorld = camera.rts;
            frame.frameNumber++;

            // Time
            double currentTime = glfwGetTime();
            lastFrameTime = currentTime - lastTime;
            lastTime = currentTime;
            totalTime += lastFrameTime;
        }

        vkDeviceWaitIdle(device);

        deletionQueue.flush();
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
    VkCommandPool commandPool;

    // Buffers and layout
    BufferBundle computeBundle;
    BufferBundle graphicsBundle;

    // Graphics
    VkRenderPass                    renderPass;
    VkPipelineLayout                graphicsPipelineLayout;
    VkPipeline                      graphicsPipeline;
    std::vector<VkCommandBuffer>    graphicsCommandBuffers;

    // Compute
    VkPipelineLayout                computePipelineLayout;
    VkPipeline                      computePipeline;
    std::vector<VkCommandBuffer>    computeCommandBuffers;
    void*                           computePushConstantReference = nullptr;
    uint32_t                        computePushConstantSize = 0;
    VkBuffer                        dispatchBuffer;
    VkDeviceMemory                  dispatchBufferMemory;

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
    Camera camera = Camera(
        glm::zero<glm::vec3>(),
        glm::zero<glm::vec3>(),
        glm::vec2(WIDTH, HEIGHT),//glm::vec2(static_cast<uint32_t>(swapChainExtent.width), static_cast<uint32_t>(swapChainExtent.height)),
        1.f,
        60.f,
        1.f,
        10.f
    );
    RTFrame frame = RTFrame{
        camera.pos,
        camera.rts,
        0
    };

    // Cleanup
    DeletionQueue deletionQueue {};

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

    void createRenderPass();
    void createGraphicsPipeline();
    void createComputePipeline();

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
};