#ifndef FLUID_GRAPHICS_VULKAN_DEVICE_HPP
#define FLUID_GRAPHICS_VULKAN_DEVICE_HPP

#include <optional>
#include <vector>

#include <vulkan/vulkan.h>

namespace fluid::graphics {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class Instance;
class Swapchain;

class Device {
public:
    void pickPhysicalDevice(Instance& p_instance, Swapchain& p_swapchain);
    void createLogicalDevice(Instance& p_instance);
    void recreateDepthResources(Swapchain& p_swapchain);
    void cleanupDepthResources();
    void cleanup();

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice pdevice, Instance& p_instance) const;
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    VkFormat findDepthFormat() const;

    VkPhysicalDevice& getPhysicalDevice() { return physicalDevice; }
    const VkPhysicalDevice& getPhysicalDevice() const { return physicalDevice; }
    VkDevice& getDevice() { return device; }
    const VkDevice& getDevice() const { return device; }

    VkQueue& getGraphicsQueue() { return graphicsQueue; }
    const VkQueue& getGraphicsQueue() const { return graphicsQueue; }
    VkQueue& getPresentQueue() { return presentQueue; }
    const VkQueue& getPresentQueue() const { return presentQueue; }

    VkImage& getDepthImage() { return depthImage; }
    const VkImage& getDepthImage() const { return depthImage; }
    VkDeviceMemory& getDepthImageMemory() { return depthImageMemory; }
    const VkDeviceMemory& getDepthImageMemory() const { return depthImageMemory; }
    VkImageView& getDepthImageView() { return depthImageView; }
    const VkImageView& getDepthImageView() const { return depthImageView; }

private:
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    bool isDeviceSuitable(VkPhysicalDevice pdevice, Instance& p_instance, Swapchain& p_swapchain) const;
    bool checkDeviceExtensionSupport(VkPhysicalDevice pdevice) const;
    VkFormat findSupportedFormat(
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features
    ) const;
    void createImage(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& memory
    ) const;
};

} // namespace fluid::graphics

#endif
