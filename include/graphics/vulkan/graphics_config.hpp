#ifndef FLUID_GRAPHICS_VULKAN_GRAPHICS_CONFIG_HPP
#define FLUID_GRAPHICS_VULKAN_GRAPHICS_CONFIG_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace fluid::graphics {

struct VulkanHostConfig {
    std::vector<std::string> requiredInstanceExtensions;
    std::function<VkResult(VkInstance, VkSurfaceKHR &)> createSurface;
    std::function<VkExtent2D()> getFramebufferExtent;
};

struct GraphicsResourceConfig {
    std::filesystem::path particleVertexShader;
    std::filesystem::path particleFragmentShader;
};

struct GraphicsRuntimeConfig {
    VulkanHostConfig vulkanHost;
    glm::vec4 clearColor = {0.015f, 0.025f, 0.055f, 1.0f};
    std::size_t particleCapacity = 100'000;
    uint32_t framesInFlight = 2;
    bool enableValidationLayers = true;
    GraphicsResourceConfig graphicsResources;
};

} // namespace fluid::graphics

#endif
