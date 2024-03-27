#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanApplicationSettings.h"
#include "command.hpp"

#include <stdexcept>
#include <vector>
#include <deque>
#include <functional>
#include <map>
#include <array>


/**
 *  Struct for storing and flushing deletion functions (deletors).
 */
struct DeletionQueue {
    std::deque<std::function<void()>> deletors;

    void addDeletor( std::function<void()>&& deletor ) {
        deletors.push_back(deletor);
    }

    void flush() {
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
            (*it)();
    }
};

/**
 *  Uniform Buffer Object memory fields.
 * 
 *  Note that with BufferBundle there should be no need to interact directly with this struct,
 *  however, it is still exposed as the objects are fully compiled and already made up for.
 */
struct UBOMemory {
    std::vector<VkBuffer>       buffers;
    std::vector<VkDeviceMemory> buffersMemory;
    std::vector<void*>          buffersMapped;
};

/**
 *  A fully compiled bundle containing buffers and associated information/functions.
 *  The deletion of said objects has already been queued by the builder.
 *  
 *  @see BufferBuilder
 */
struct BufferBundle {
    // Descriptors
    VkDescriptorSetLayout        descriptorSetLayout;
    VkDescriptorPool             descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    // Memory
    std::map<uint32_t, UBOMemory> uboMemories;

    /**
     *  Updates the contents of a Uniform Buffer Object.
     *  
     *  @param binding Binding, as in shader-code.
     *  @param data The new data.
     *  @param frames Which frames to update for. If left blank or with only a single "-1", updates all frames.
     */
    template<typename T>
    void updateUBO(
        uint32_t            binding,
        T                   & data,
        std::vector<int>    frames = std::vector<int> {-1}
    ) {
        // If the binding does not correspond to any known UBO, throw an error
        if (uboMemories.find(binding) == uboMemories.end())
            throw std::runtime_error("ERR::VULKAN::UPDATE_UBO::INVALID_BINDING");

        // If no frames are selected for updating, return
        if (frames.size() == 0)
            return;

        // If the only frame selected for updating is "-1", update all frames
        if (frames.size() == 1 && frames[0] == -1) {
            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
                memcpy(uboMemories[binding].buffersMapped[i], &data, sizeof(T));
            return;
        }

        // Otherwise, update specified frames
        for (auto i : frames) {
            // (if frame-to-update is outside of bounds, throw an error)
            if (i >= MAX_FRAMES_IN_FLIGHT)
                throw std::runtime_error("ERR::VULKAN::UPDATE_UBO::INVALID_FRAME");
            memcpy(uboMemories[binding].buffersMapped[i], &data, sizeof(T));
        }
    }
};

/**
 *  Class for creating and building buffer objects into a fully compiled BufferBundle.
 *  Handles creation and deletion of said objects. Last step is always to build().
 */
class BufferBuilder {
public:
    /**
     *  Constructor.
     */
    BufferBuilder(
        VkPhysicalDevice physicalDevice,
        VkDevice         device,
        DeletionQueue*   deletionQueue
    ) : physicalDevice(physicalDevice), device(device), deletionQueue(deletionQueue) {}

    /**
     *  Creates and adds a Uniform Buffer Object to the layout.
     *  Allocates memory and binds it to the appropriate fields in uboMemories.
     * 
     *  @param binding Binding, as in shader-code.
     *  @param stageFlags Which stages the UBO should be visible to.
     *  @param physicalDevice The Vulkan physical device.
     *  @param device The Vulkan logical device.
     * 
     *  @return itself, for functional purposes.
     */
    template<typename T>
    BufferBuilder UBO (
        uint32_t            binding,
        VkShaderStageFlags  stageFlags
    ) {
        // Create and push layout bindings
        layoutBindings.push_back(
            VkDescriptorSetLayoutBinding{ binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, stageFlags, nullptr }
        );

        // Add pool sizes
        addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        // Create buffers
        Buffer ubo (
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            true,
            std::vector<T>{},
            physicalDevice,
            device,
            NULL,
            NULL
        );
        UBOMemory uboMemory { ubo.buffers, ubo.buffersMemory, ubo.buffersMapped };
        uboMemories[binding] = uboMemory;

        deletionQueue->addDeletor([=]() {
            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroyBuffer(device, uboMemory.buffers[i], nullptr);
                vkFreeMemory(device, uboMemory.buffersMemory[i], nullptr);
            }
        });

        // Create and push descriptor writes
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo *bufferInfo = new VkDescriptorBufferInfo {};
            bufferInfo->buffer = uboMemory.buffers[i];
            bufferInfo->offset = 0;
            bufferInfo->range = sizeof(T);

            VkWriteDescriptorSet write {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstBinding = binding;
            write.dstArrayElement = 0;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.pBufferInfo = bufferInfo;
            
            descriptorWrites[i].push_back(write);
        }

        return *this;
    };

    /**
     *  Builds the Buffer Bundle and queues the deletion of created objects.
     */
    BufferBundle build() {
        // Build
        createDescriptorSetLayout();
        createDescriptorPool();
        createDescriptorSets();

        // Make bundle and return
        BufferBundle bundle{};
        bundle.descriptorSetLayout = descriptorSetLayout;
        bundle.descriptorPool = descriptorPool;
        bundle.descriptorSets = descriptorSets;
        bundle.uboMemories = uboMemories;
        return bundle;
    }

private:
    // Environment
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    DeletionQueue* deletionQueue;

    // Descriptors
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings {};
    std::vector<VkDescriptorPoolSize> poolSizes {};
    std::array<std::vector<VkWriteDescriptorSet>, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)> descriptorWrites {};

    // Memory
    std::map<uint32_t, UBOMemory> uboMemories;

    // Calculated properties
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    /**
     *  Adds a pool size to poolSizes.
     *  If the type already exists, increments its descriptorCount instead.
     */
    void addPoolSize( VkDescriptorType type ) {
        for (size_t i = 0; i < poolSizes.size(); i++)
            if (poolSizes[i].type == type) {
                poolSizes[i].descriptorCount += static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
                return;
            }
        poolSizes.push_back(
            VkDescriptorPoolSize{ type, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) }
        );
    }

    /**
     *  Creates the Descriptor Set Layout for the layout.
     *  This should be done only once.
     */
    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
        layoutInfo.pBindings = layoutBindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("ERR::VULKAN::CREATE_DESCRIPTOR_SET_LAYOUT::CREATION_FAILED");

        deletionQueue->addDeletor([=]() {
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        });
    }

    /**
     *  Creates the Descriptor Pool for the layout.
     *  This should be done only once.
     */
    void createDescriptorPool() {
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
            throw std::runtime_error("ERR::VULKAN::CREATE_DESCRIPTOR_POOL::CREATION_FAILED");
        
        deletionQueue->addDeletor([=]() {
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        });
    }

    /**
     *  Creates the Descriptor Sets for the layout.
     *  This should only be done after the Descriptor Pool and Descriptor Set Layout has been created.
     *
     *  @see VKLayout::createDescriptorSetLayout(...)
     *  @see VKLayout::createDescriptorPool(...)
     */
    void createDescriptorSets() {
        // Prepare as many descriptor sets as there are frames-in-flight
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        // Allocate the descriptors
        // (these are automatically destroyed when pool is deleted)
        // (Also, if createDescriptorPool is wrong, this might not give any warnings)
        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        auto err = vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());
        if (err != VK_SUCCESS)
            throw std::runtime_error("ERR::VULKAN::CREATE_DESCRIPTOR_SETS::DESCRIPTOR_SETS_ALLOCATION_FAILED");
    
        // Update descriptor sets
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            for (size_t j = 0; j < descriptorWrites[i].size(); j++)
                descriptorWrites[i][j].dstSet = descriptorSets[i];
            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites[i].size()), descriptorWrites[i].data(), 0, nullptr);
            for (size_t j = 0; j < descriptorWrites[i].size(); j++) // cleanup
                delete descriptorWrites[i][j].pBufferInfo;
        }
    }
};

/**
 *  Class for creating a buffer object, i.e. UBO or SSBO.
 *  Does NOT handle the deletion of its buffers and buffer memories.
 */
class Buffer {
public:
    // Exposed fields
    std::vector<VkBuffer>       buffers;
    std::vector<VkDeviceMemory> buffersMemory;
    std::vector<void*>          buffersMapped;

    /**
     *  Complete constructor.
     *  Allocates memory for the buffer and its fields, which are stored in the exposed fields.
     * 
     *  @param mainUsage The main usage of the buffer, i.e. VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT for Uniform Buffer Objects.
     *  @param isMapped Whether the buffer should be consistently mapped and host visible.
     *  @param initialData The initial data of the buffer, if any. For UBOs, the vector's first element is used.
     *  @param physicalDevice The Vulkan physical device.
     *  @param device The Vulkan logical device.
     *  @param commandPool A command pool to pull commands from.
     *  @param queue The queue to commit commands to.
     */
    template<typename T>
    Buffer (
        VkBufferUsageFlags  mainUsage,
        bool                isMapped,
        std::vector<T>      initialData,
        VkPhysicalDevice    physicalDevice,
        VkDevice            device,
        VkCommandPool       commandPool,
        VkQueue             queue
    ) {
        VkMemoryPropertyFlags   props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VkBufferUsageFlags      usage = mainUsage;
        VkBuffer                stagingBuffer;
        VkDeviceMemory          stagingBufferMemory;
        VkDeviceSize            bufferSize = sizeof(T);

        // If the buffer is mapped, make it host visible and coherent
        if (isMapped) {
            props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            buffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
        }
        // If the buffer is not mapped and has initial data, stage it
        else if (initialData.size() > 0) {
            bufferSize *= initialData.size();
            usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            
            // Create staging buffer
            createBuffer(
                bufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                physicalDevice, device,
                stagingBuffer, stagingBufferMemory
            );

            // Copy data to staging buffer
            void* ptr;
            vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &ptr);
            memcpy(ptr, initialData.data(), (size_t)bufferSize);
            vkUnmapMemory(device, stagingBufferMemory);
        }

        // Create main buffers (and transfer data)
        buffers.resize(MAX_FRAMES_IN_FLIGHT);
        buffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer( bufferSize, usage, props, physicalDevice, device, buffers[i], buffersMemory[i] );

            if (isMapped)
                vkMapMemory(device, buffersMemory[i], 0, bufferSize, 0, &buffersMapped[i]);

            if (isMapped && initialData.size() > 0)
                memcpy(buffersMapped[i], &initialData[0], sizeof(T));

            if (!isMapped && initialData.size() > 0)
                copyBuffer( stagingBuffer, buffers[i], bufferSize, device, commandPool, queue );
        }

        // Cleanup
        if (!isMapped && initialData.size() > 0) {
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
        }
    }
};

/**
 *  Locates memory of a given type and props.
 */
uint32_t inline findMemoryType(
    uint32_t                typeFilter,
    VkMemoryPropertyFlags   properties,
    VkPhysicalDevice        physicalDevice
) {
    // Fetch memory props
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    /*
        We only care about memoryTypes, not the memoryHeaps.
        Although memoryHeaps can be used to select based on type of memory (VRAM vs RAM/swapspace).

        each memoryType has property flags which include:
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT - CPU can map and read/write to memory.
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT - Mapped data is immidiately unmappable, no need to worry about waiting for it.
    */

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        // Pick first viable memory type
        // (check if bits in typeFilter are set to "1" for the given memorytype)
        // (verify that the correct properties are present)
        if ( typeFilter & (1 << i) &&
             (memProperties.memoryTypes[i].propertyFlags & properties) == properties
        ) return i;
    }

    throw std::runtime_error("ERR::VULKAN::FIND_MEMORY_TYPE::NONE_FOUND");
}

/**
 *  Creates a buffer.
 *
 *  @param size The size of the buffer in bytes.
 *  @param usage Usage type of the buffer, i.e. VK_BUFFER_USAGE_VERTEX_BUFFER_BIT for VBO.
 *  @param properties Memory properties to request, i.e. VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT for VBO.
 *  @param physicalDevice The Vulkan physical device.
 *  @param device The Vulkan logical device.
 *  @param buffer Empty variable for buffer.
 *  @param bufferMemory Empty variable for buffer memory.
 */
void inline createBuffer(
    VkDeviceSize            size,
    VkBufferUsageFlags      usage,
    VkMemoryPropertyFlags   properties,
    VkPhysicalDevice        physicalDevice,
    VkDevice                device,
    VkBuffer                & buffer,
    VkDeviceMemory          & bufferMemory
) {
    // Create buffer object
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // will ONLY be used from graphics queue
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::CREATE_BUFFER::CREATION_FAILED");

    // Set up memory requirements and alloc info
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);

    /*
        Allocating memory for and every buffer if not good!
        The maxMemoryAllocationCount is very low.

        Use something like this instead in the future: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.
        Even better, follow these guidelines: https://developer.nvidia.com/vulkan-memory-management
        (In short, VBOs and EBO/IBO should be allocated into the SAME buffer. This allows the driver to re-use chunks of memory! AKA "aliasing".)
    */
    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        throw std::runtime_error("ERR::VULKAN::CREATE_BUFFER::ALLOCATION_FAILED");

    // Assign newly allocated memory to vbo
    // (for future reference: if last parameter is not 0, memory size must be aligned with memRequirements.alignments)
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

/**
 *  Copies the contents of a buffer onto another.
 *  Note: VK_BUFFER_USAGE_TRANSFER_SRC_BIT and VK_BUFFER_USAGE_TRANSFER_DST_BIT are required for the source and recipient, respectively.
 * 
 *  @param srcBuffer The buffer to copy from.
 *  @param dstBuffer The buffer to copy to.
 *  @param size How many bytes of the source buffer to copy (should be its size in most cases).
 *  @param device The Vulkan logical device.
 *  @param commandPool The command pool to use.
 *  @param queue The queue to copy-command with.
 */
void inline copyBuffer(
    VkBuffer        srcBuffer,
    VkBuffer        & dstBuffer,
    VkDeviceSize    size,
    VkDevice        device,
    VkCommandPool   commandPool,
    VkQueue         queue
) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer, commandPool, device, queue);
}

/**
 *  Creates a uniform buffer object.
 *  TODO: DELETE, DEPRECATED.
 *  
 *  @param physicalDevice The Vulkan physical device.
 *  @param device The Vulkan logical device.
 *  @param buffers Empty vector for buffers.
 *  @param buffersMemory Empty vector for the buffers' memory.
 *  @param buffersMapped Empty vector for host visible buffer memory.
 */
template <typename T>
void inline createUniformBuffers(
    VkPhysicalDevice            physicalDevice,
    VkDevice                    device,
    std::vector<VkBuffer>       & buffers,
    std::vector<VkDeviceMemory> & buffersMemory,
    std::vector<void*>          & buffersMapped
) {
    Buffer buffer(
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        true,
        std::vector<T>{},
        physicalDevice,
        device,
        NULL,
        NULL
    );

    buffers = buffer.buffers;
    buffersMemory = buffer.buffersMemory;
    buffersMapped = buffer.buffersMapped;
}

/**
 *  Creates a set of SSBOs with the given data.
 *  TODO: DELETE, DEPRECATED.
 */
template <typename T>
void inline createSSBO(
    VkPhysicalDevice            physicalDevice,
    VkDevice                    device,
    VkCommandPool               commandPool,
    VkQueue                     queue,
    std::vector<T>              data,
    std::vector<VkBuffer>       & buffers,
    std::vector<VkDeviceMemory> & buffersMemory
) {
    Buffer buffer(
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        false,
        data,
        physicalDevice,
        device,
        commandPool,
        queue
    );

    buffers = buffer.buffers;
    buffersMemory = buffer.buffersMemory;
}