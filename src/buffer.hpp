#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanApplicationSettings.h"
#include "command.hpp"
#include "memory.hpp"
#include "image.hpp"

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
 *  Buffer memory fields.
 * 
 *  Note that with BufferBundle there should be no need to interact directly with this struct,
 *  however, it is still exposed as the objects are fully compiled and already made up for.
 */
struct BufferMemory {
    std::vector<VkBuffer>       buffers;
    std::vector<VkDeviceMemory> buffersMemory;
    std::vector<void*>          buffersMapped;
};

/**
 *  Image memory fields.
 *  TODO: CHECK IF AN IMAGE IS REQUIRED FOR EACH IN-FLIGHT-FRAME¨.
 */
struct ImageMemory {
    VkImage         image;
    VkImageView     imageView;
    VkDeviceMemory  imageMemory;
    VkSampler       sampler;
    VkImageLayout   layout;
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
    std::map<uint32_t, BufferMemory> bufferMemories;
    std::map<uint32_t, ImageMemory>  imageMemories;

    /**
     *  Updates the contents of a Uniform Buffer Object.
     *  
     *  @param binding Binding, as in shader-code.
     *  @param data The new data.
     *  @param frames Which frames to update for. If left blank or with only a single "-1", updates all frames.
     */
    template<typename T>
    void updateBuffer(
        uint32_t            binding,
        std::vector<T>      data,
        std::vector<int>    frames = std::vector<int> {-1}
    ) {
        // Early error catching
        if (bufferMemories.find(binding) == bufferMemories.end())
            throw std::runtime_error("ERR::VULKAN::UPDATE_BUFFER::INVALID_BINDING");
        if (bufferMemories[binding].buffersMapped.size() == 0)
            throw std::runtime_error("ERR::VULKAN::UPDATE_BUFFER::UPDATING_UNMAPPED_CURRENTLY_NOT_SUPPORTED");

        // If no frames are selected for updating, return
        if (frames.size() == 0)
            return;

        // If the only frame selected for updating is "-1", update all frames
        if (frames.size() == 1 && frames[0] == -1) {
            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
                memcpy(bufferMemories[binding].buffersMapped[i], data.data(), sizeof(T) * data.size());
            return;
        }

        // Otherwise, update specified frames
        for (auto i : frames) {
            // (if frame-to-update is outside of bounds, throw an error)
            if (i >= MAX_FRAMES_IN_FLIGHT)
                throw std::runtime_error("ERR::VULKAN::UPDATE_BUFFER::INVALID_FRAME");
            memcpy(bufferMemories[binding].buffersMapped[i], data.data(), sizeof(T) * data.size());
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
        VkCommandPool    commandPool,
        VkQueue          queue,
        DeletionQueue*   deletionQueue
    ) : physicalDevice(physicalDevice), device(device), commandPool(commandPool), queue(queue), deletionQueue(deletionQueue) {}

    /**
     *  Creates and adds a Uniform Buffer Object to the layout.
     *  Allocates memory and binds it to the appropriate fields in bufferMemories.
     * 
     *  @param binding Binding, as in shader-code.
     *  @param stageFlags Which stages the UBO should be visible to.
     *  @param initialData Initial data.
     * 
     *  @return itself, for functional purposes.
     */
    template<typename T>
    BufferBuilder UBO (
        uint32_t            binding,
        VkShaderStageFlags  stageFlags,
        std::vector<T>      initialData
    ) {
        return genericBuffer<T>(
            binding,
            stageFlags,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            true,
            initialData
        );
    };

    /**
     *  Creates and adds a Uniform Buffer Object to the layout.
     *  Allocates memory and binds it to the appropriate fields in bufferMemories.
     * 
     *  @param binding Binding, as in shader-code.
     *  @param stageFlags Which stages the UBO should be visible to.
     *  @param initialData Initial data.
     * 
     *  @return itself, for functional purposes.
     */
    template<typename T>
    BufferBuilder SSBO (
        uint32_t            binding,
        VkShaderStageFlags  stageFlags,
        std::vector<T>      initialData
    ) {
        return genericBuffer<T>(
            binding,
            stageFlags,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            false,
            initialData
        );
    };

    /**
     *  Creates and adds a buffer object to the layout.
     *  Allocates memory and binds it to the appropriate fields in bufferMemories.
     * 
     *  @param binding Binding, as in shader-code.
     *  @param stageFlags Which stages the buffer should be visible to.
     *  @param mainUsage The main usage of the buffer, i.e. VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT for Uniform Buffer Objects.
     *  @param isMapped Whether the buffer should be consistently mapped and host visible.
     *  @param initialData The initial data of the buffer, if any.
     *
     *  @return itself, for functional purposes.
     */
    template<typename T>
    BufferBuilder genericBuffer (
        uint32_t            binding,
        VkShaderStageFlags  stageFlags,
        VkDescriptorType    type,
        VkBufferUsageFlags  mainUsage,
        bool                isMapped,
        std::vector<T>      initialData
    ) {
        // Create and push layout bindings
        layoutBindings.push_back(
            VkDescriptorSetLayoutBinding{ binding, type, 1, stageFlags, nullptr }
        );

        // Add pool sizes
        addPoolSize(type);

        // Create buffers
        Buffer buffer (
            mainUsage,
            isMapped,
            initialData,
            physicalDevice,
            device,
            commandPool,
            queue
        );
        BufferMemory bufferMemory { buffer.buffers, buffer.buffersMemory, buffer.buffersMapped };
        bufferMemories[binding] = bufferMemory;

        deletionQueue->addDeletor([=]() {
            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroyBuffer(device, bufferMemory.buffers[i], nullptr);
                vkFreeMemory(device, bufferMemory.buffersMemory[i], nullptr);
            }
        });

        // Create and push descriptor writes
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo *bufferInfo = new VkDescriptorBufferInfo {};
            bufferInfo->buffer = bufferMemory.buffers[i];
            bufferInfo->offset = 0;
            bufferInfo->range = sizeof(T) * initialData.size();

            VkWriteDescriptorSet write {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstBinding = binding;
            write.dstArrayElement = 0;
            write.descriptorCount = 1;
            write.descriptorType = type;
            write.pBufferInfo = bufferInfo;
            
            descriptorWrites[i].push_back(write);
        }

        return *this;
    }

    /**
     *  (Creates and) Adds a sampled image to the layout.
     *  
     *  @param binding Binding, as in shader-code.
     *  @param stageFlags Which stages the buffer should be visible to.
     *  @param existingImage Image memory to create the sampler for. If left empty, an image is created.
     *  @param width If creating an image, the width of that image.
     *  @param height If creating an image, the height of that image.
     * 
     *  @return itself, for functional purposes.
     */
    BufferBuilder sampler (
        uint32_t            binding,
        VkShaderStageFlags  stageFlags,
        ImageMemory*        existingImage = nullptr,
        uint32_t            width = NULL,
        uint32_t            height = NULL
    ) {
        return genericImage (
            binding,
            stageFlags,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            true,
            false,
            existingImage,
            width,
            height
        );
    }

    /**
     *  (Creates and) Adds a storage image to the layout.
     *  
     *  @param binding Binding, as in shader-code.
     *  @param stageFlags Which stages the buffer should be visible to.
     *  @param existingImage Image memory to create the layout for. If left empty, an image is created.
     *  @param width If creating an image, the width of that image.
     *  @param height If creating an image, the height of that image.
     * 
     *  @return itself, for functional purposes.
     */
    BufferBuilder storageImage (
        uint32_t            binding,
        VkShaderStageFlags  stageFlags,
        ImageMemory*        existingImage = nullptr,
        uint32_t            width = NULL,
        uint32_t            height = NULL
    ) {
        return genericImage (
            binding,
            stageFlags,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            false,
            true,
            existingImage,
            width,
            height
        );
    }

    /**
     *  (Creates and) Adds an image to the layout.
     *  Useful for creating images which are used for both storage and sampling.
     *  
     *  @param binding Binding, as in shader-code.
     *  @param stageFlags Which stages the buffer should be visible to.
     *  @param type The type of image resource to create the layout, i.e. VK_DESCRIPTOR_TYPE_STORAGE_IMAGE for storage images.
     *  @param sampled Whether a sampler should be made for the image.
     *  @param storage Whether the layout should support being a storage image.
     *  @param existingImage Image memory to create the layout for. If left empty, an image is created.
     *  @param width If creating an image, the width of that image.
     *  @param height If creating an image, the height of that image.
     * 
     *  @return itself, for functional purposes.
     */
    BufferBuilder genericImage (
        uint32_t            binding,
        VkShaderStageFlags  stageFlags,
        VkDescriptorType    type,
        bool                sampled,
        bool                storage,
        ImageMemory*        existingImage = nullptr,
        uint32_t            width = NULL,
        uint32_t            height = NULL
    ) {
        // Early error handling
        if (!sampled && !storage)
            throw std::runtime_error("ERR::VULKAN::GENERIC_IMAGE::IMAGE_MUST_BE_SAMPLED_OR_STORAGE");
        if (existingImage == nullptr && (width == NULL || height == NULL))
            throw std::runtime_error("ERR::VULKAN::GENERIC_IMAGE::IMAGE_NEITHER_EXISTING_OR_VALID_DIMENSIONS");
        if (existingImage != nullptr && existingImage->sampler == NULL && sampled)
            throw std::runtime_error("ERR::VULKAN::GENERIC_IMAGE::ADDING_SAMPLERS_TO_EXISTING_IMAGES_NOT_SUPPORTED");

        // Create and push layout bindings
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = type;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = stageFlags;
        layoutBinding.pImmutableSamplers = nullptr;
        layoutBindings.push_back( layoutBinding );

        // Add pool sizes
        addPoolSize(type);

        // If image does not exist, create it
        if (existingImage == nullptr) {
            imageMemories[binding] = ImageMemory{};
            existingImage = &imageMemories[binding];

            //properties
            existingImage->layout = storage ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            VkImageUsageFlags usage = sampled ? (storage ? VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT : VK_IMAGE_USAGE_SAMPLED_BIT) : VK_IMAGE_USAGE_STORAGE_BIT;

            //image
            createImage (
                width, height, 1,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_TILING_OPTIMAL,
                usage,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                physicalDevice,
                device,
                existingImage->image,
                existingImage->imageMemory
            );

            //layout
            transitionImageLayout (
                existingImage->image,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_LAYOUT_UNDEFINED,
                existingImage->layout,
                1,
                device,
                commandPool,
                queue
            );

            //view
            existingImage->imageView = createImageView (
                existingImage->image,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_ASPECT_COLOR_BIT, 
                1, 
                device
            );

            //sampler
            if (sampled) createSampler(physicalDevice, device, existingImage->sampler);

            //cleanup
            // TODO: MOVE THIS INTO EACH OF THE CORRESPONDING CREATION FUNCTIONS
            VkImageView imageView = existingImage->imageView;
            VkImage image = existingImage->image;
            VkDeviceMemory imageMemory = existingImage->imageMemory;
            VkSampler sampler = existingImage->sampler;

            deletionQueue->addDeletor([=]() {
                vkDestroyImageView(device, imageView, nullptr);
                vkDestroyImage(device, image, nullptr);
                vkFreeMemory(device, imageMemory, nullptr);
                vkDestroySampler(device, sampler, nullptr);
            });
        }
        // If the image already exists, verify and bind it
        else {
            if (existingImage->imageView == NULL)
                throw std::runtime_error("ERR::VULKAN::STORAGE_IMAGE::EXISTING_IMAGE_IS_INVALID");
            imageMemories[binding] = *existingImage;
        }

        // Create and push descriptor writes
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorImageInfo* imageInfo = new VkDescriptorImageInfo{};
            imageInfo->imageLayout = existingImage->layout;
            imageInfo->imageView = existingImage->imageView;
            imageInfo->sampler = existingImage->sampler;

            VkWriteDescriptorSet write {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstBinding = binding;
            write.dstArrayElement = 0;
            write.descriptorCount = 1;
            write.descriptorType = type;
            write.pImageInfo = imageInfo;
            
            descriptorWrites[i].push_back(write);
        }

        return *this;
    }

    /**
     *  Builds the Buffer Bundle.
     */
    BufferBundle build() {
        // Build
        createDescriptorSetLayout();
        createDescriptorPool();
        createDescriptorSets();

        // Make bundle and return
        BufferBundle bundle{};
        bundle.descriptorSetLayout  = descriptorSetLayout;
        bundle.descriptorPool       = descriptorPool;
        bundle.descriptorSets       = descriptorSets;
        bundle.bufferMemories       = bufferMemories;
        bundle.imageMemories        = imageMemories;
        return bundle;
    }

private:
    // Environment
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    DeletionQueue* deletionQueue;
    VkCommandPool commandPool;
    VkQueue queue;

    // Descriptors
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings {};
    std::vector<VkDescriptorPoolSize> poolSizes {};
    std::array<std::vector<VkWriteDescriptorSet>, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)> descriptorWrites {};

    // Memory
    std::map<uint32_t, BufferMemory> bufferMemories;
    std::map<uint32_t, ImageMemory>  imageMemories;

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
            for (size_t j = 0; j < descriptorWrites[i].size(); j++) { //cleanup
                if (descriptorWrites[i][j].pBufferInfo != nullptr) delete descriptorWrites[i][j].pBufferInfo;
                if (descriptorWrites[i][j].pImageInfo != nullptr)  delete descriptorWrites[i][j].pImageInfo;
            }

        }
    }
};