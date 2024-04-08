#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint> // uint32_t
#include <vector>

static const uint32_t  WIDTH = 1152;
static const uint32_t  HEIGHT = 768;
static const int       MAX_FRAMES_IN_FLIGHT = 2;

static const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

static const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};