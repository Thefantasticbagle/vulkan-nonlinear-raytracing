/**
 *  THIS FILE IS NOT IN USE YET.
 *  IT IS GOING TO CONTAIN ACCLERATION STRUCTURE AND RAY QUERY FUNCTIONS.
 */

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

// vkGetAccelerationStructureBuildSizesKHR
static void GetAccelerationStructureBuildSizesKHR(
    VkInstance                                  instance,
    VkDevice                                    device,
    VkAccelerationStructureBuildTypeKHR         buildType,
    const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
    const uint32_t*                             pMaxPrimitiveCounts,
    VkAccelerationStructureBuildSizesInfoKHR*   pSizeInfo
) {

    auto func = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetInstanceProcAddr(instance, "vkGetAccelerationStructureBuildSizesKHR");
    if (func != nullptr) return func(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
}

// vkCreateAccelerationStructureKHR
static VkResult CreateAccelerationStructureKHR(
    VkInstance                                  instance,
    VkDevice                                    device,
    const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkAccelerationStructureKHR*                 pAccelerationStructure
) {

    auto func = (PFN_vkCreateAccelerationStructureKHR)vkGetInstanceProcAddr(instance, "vkCreateAccelerationStructureKHR");
    if (func != nullptr) return func(device, pCreateInfo, pAllocator, pAccelerationStructure);
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

// vkCreateAccelerationStructureKHR
static void CmdBuildAccelerationStructuresKHR(
    VkInstance                                  instance,
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
    const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos
) {

    auto func = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetInstanceProcAddr(instance, "vkCmdBuildAccelerationStructuresKHR");
    if (func != nullptr) return func(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
    return;
}

//void inline makeAccelerationStructure(
//    VkInstance instance,
//    VkPhysicalDevice physicalDevice,
//    VkDevice device,
//    VkCommandPool commandPool,
//    VkQueue queue
//) {
//    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);
//
//    // --- Determine sizes required for acceleration structure
//    // Set up bottom-level geometries
//    std::vector<VkAccelerationStructureGeometryKHR> geometries{};
//    VkAccelerationStructureGeometryAabbsDataKHR aabbs{};
//    aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
//    aabbs.data.deviceAddress = 0;
//    aabbs.data.hostAddress = nullptr;
//    aabbs.stride = sizeof(VkAabbPositionsKHR);
//
//    VkAccelerationStructureGeometryKHR geometry{};
//    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
//    geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
//    geometry.geometry.aabbs = aabbs;
//    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
//    geometries.push_back(geometry);
//
//    // Create uncomplete geometry info struct
//    // (For now, only acceleration structure type, geometry types, counts, and maximum sizes needs to be populated)
//    VkAccelerationStructureBuildGeometryInfoKHR geometryInfo{};
//    geometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
//    geometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR; // VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
//    geometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR; // Prefer fast tracing rather than building
//    geometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR; // Use VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR for updating
//    geometryInfo.geometryCount = static_cast<uint32_t>(geometries.size());
//    geometryInfo.pGeometries = geometries.data();
//
//    // Query sizes
//    VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
//    buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
//    GetAccelerationStructureBuildSizesKHR(
//        instance,
//        device,
//        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
//        &geometryInfo,
//        &geometryInfo.geometryCount,
//        &buildSizesInfo
//    );
//
//    // Create buffers
//    VkBuffer        accelerationStructureBuffer;
//    VkDeviceMemory  accelerationStructureBufferMemory;
//    createBuffer(
//        buildSizesInfo.accelerationStructureSize,
//        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
//        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//        physicalDevice,
//        device,
//        accelerationStructureBuffer,
//        accelerationStructureBufferMemory
//    );
//
//    VkBuffer        buildScratchBuffer;
//    VkDeviceMemory  buildScratchBufferMemory;
//    createBuffer(
//        buildSizesInfo.buildScratchSize,
//        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
//        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//        physicalDevice,
//        device,
//        buildScratchBuffer,
//        buildScratchBufferMemory,
//        true
//    );
//
//    VkBufferDeviceAddressInfo addressInfo{};
//    addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
//    addressInfo.buffer = buildScratchBuffer;
//    geometryInfo.scratchData.deviceAddress = vkGetBufferDeviceAddress(device, &addressInfo);
//
//    // --- Create acceleration structure object and link objects
//    VkAccelerationStructureCreateInfoKHR accelerationStructureInfo{};
//    accelerationStructureInfo.sType     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
//    accelerationStructureInfo.type      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR; // VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
//    accelerationStructureInfo.size      = buildSizesInfo.accelerationStructureSize; // ???
//    accelerationStructureInfo.offset    = 0;
//    accelerationStructureInfo.buffer    = accelerationStructureBuffer;
//
//    VkAccelerationStructureKHR accelerationStructure;
//    CreateAccelerationStructureKHR(
//        instance,
//        device, 
//        &accelerationStructureInfo,
//        nullptr, 
//        &accelerationStructure
//    );
//
//    geometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
//    geometryInfo.dstAccelerationStructure = accelerationStructure;
//
//    // --- Determine ranges and build acceleration structure
//    VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
//    rangeInfo.primitiveCount    = 1;
//    rangeInfo.primitiveOffset   = 0;
//    rangeInfo.firstVertex       = 0;
//    rangeInfo.transformOffset   = 0;
//    const VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;
//
//    CmdBuildAccelerationStructuresKHR(
//        instance,
//        commandBuffer,
//        1,
//        &geometryInfo,
//        &pRangeInfo 
//    );
//
//    endSingleTimeCommands(commandBuffer, commandPool, device, queue);
//}


void inline makeAccelerationStructure(
    VkInstance instance,
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkCommandPool commandPool,
    VkQueue queue
) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    // --- Build acceleration structure
    // Set up bottom-level geometries
    std::vector<VkAccelerationStructureGeometryKHR> geometries{};
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> rangeInfos{};

    {
        std::vector<VkAabbPositionsKHR> aabbsData{};
        aabbsData.push_back(VkAabbPositionsKHR{ 0.f,0.f,0.f,0.f,0.f,0.f });

        VkAccelerationStructureGeometryAabbsDataKHR aabbs{};
        aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
        //aabbs.data.deviceAddress = 0;
        aabbs.data.hostAddress = aabbsData.data();
        aabbs.stride = sizeof(VkAabbPositionsKHR);

        VkAccelerationStructureGeometryKHR geometry{};
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
        geometry.geometry.aabbs = aabbs;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

        geometries.push_back(geometry);

        VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
        rangeInfo.primitiveCount    = 1;
        rangeInfo.primitiveOffset   = 0;
        rangeInfo.firstVertex       = 0;
        rangeInfo.transformOffset   = 0;

        rangeInfos.push_back(rangeInfo);
    }

    // Create uncomplete geometry info struct
    // (For now, only acceleration structure type, geometry types, counts, and maximum sizes needs to be populated)
    VkAccelerationStructureBuildGeometryInfoKHR geometryInfo{};
    geometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    geometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR; // VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    geometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR; // Prefer fast tracing rather than building
    geometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR; // Use VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR for updating
    geometryInfo.geometryCount = static_cast<uint32_t>(geometries.size());
    geometryInfo.pGeometries = geometries.data();

    // Query sizes
    VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
    buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    GetAccelerationStructureBuildSizesKHR(
        instance,
        device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &geometryInfo,
        &geometryInfo.geometryCount,
        &buildSizesInfo
    );

    // Create acceleration structure and its buffer
    VkBuffer        accelerationStructureBuffer;
    VkDeviceMemory  accelerationStructureBufferMemory;
    createBuffer(
        buildSizesInfo.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        physicalDevice,
        device,
        accelerationStructureBuffer,
        accelerationStructureBufferMemory
    );

    VkAccelerationStructureCreateInfoKHR accelerationStructureInfo{};
    accelerationStructureInfo.sType     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureInfo.type      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR; // VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationStructureInfo.size      = buildSizesInfo.accelerationStructureSize; // ???
    accelerationStructureInfo.offset    = 0;
    accelerationStructureInfo.buffer    = accelerationStructureBuffer;

    VkAccelerationStructureKHR accelerationStructure;
    CreateAccelerationStructureKHR(
        instance,
        device, 
        &accelerationStructureInfo,
        nullptr, 
        &accelerationStructure
    );

    // --- Create scratch buffer
    VkBuffer        buildScratchBuffer;
    VkDeviceMemory  buildScratchBufferMemory;
    createBuffer(
        buildSizesInfo.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        physicalDevice,
        device,
        buildScratchBuffer,
        buildScratchBufferMemory,
        true
    );

    VkBufferDeviceAddressInfo buildScratchBufferAddressInfo{};
    buildScratchBufferAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    buildScratchBufferAddressInfo.buffer = buildScratchBuffer;
    VkDeviceAddress buildScratchBufferAddress = vkGetBufferDeviceAddress(device, &buildScratchBufferAddressInfo);

    VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
    buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildGeometryInfo.dstAccelerationStructure = accelerationStructure;
    buildGeometryInfo.geometryCount = static_cast<uint32_t>(geometries.size());
    buildGeometryInfo.pGeometries = geometries.data();
    buildGeometryInfo.scratchData.deviceAddress = buildScratchBufferAddress;

    auto ppRangeInfos = &*rangeInfos.data();
    CmdBuildAccelerationStructuresKHR(
        instance,
        commandBuffer,
        1,
        &buildGeometryInfo,
        &ppRangeInfos
    );

    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    endSingleTimeCommands(commandBuffer, commandPool, device, queue);
}