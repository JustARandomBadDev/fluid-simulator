#ifndef FLUID_GRAPHICS_VULKAN_VULKAN_BUFFER_HPP
#define FLUID_GRAPHICS_VULKAN_VULKAN_BUFFER_HPP

#include <vulkan/vulkan.h>

namespace fluid::graphics {

class Device;

class VulkanBuffer {
  public:
    VulkanBuffer() = default;
    ~VulkanBuffer();
    VulkanBuffer(const VulkanBuffer &) = delete;
    VulkanBuffer &operator=(const VulkanBuffer &) = delete;
    VulkanBuffer(VulkanBuffer &&other) noexcept;
    VulkanBuffer &operator=(VulkanBuffer &&other) noexcept;

    void createBuffer(
        VkDeviceSize psize,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        Device &p_device
    );
    void *map(VkDeviceSize offset = 0, VkDeviceSize mappedSize = VK_WHOLE_SIZE);
    void unmap();
    void cleanup();

    VkBuffer &getBuffer() {
        return buffer;
    }
    const VkBuffer &getBuffer() const {
        return buffer;
    }
    VkDeviceMemory &getBufferMemory() {
        return bufferMemory;
    }
    const VkDeviceMemory &getBufferMemory() const {
        return bufferMemory;
    }
    VkDeviceSize getSize() const {
        return size;
    }
    VkBufferUsageFlags getUsage() const {
        return usageFlags;
    }
    VkMemoryPropertyFlags getMemoryProperties() const {
        return memoryProperties;
    }

  private:
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
    VkBufferUsageFlags usageFlags = 0;
    VkMemoryPropertyFlags memoryProperties = 0;
    Device *_device = nullptr;
    void *_mapped = nullptr;
};

} // namespace fluid::graphics

#endif
