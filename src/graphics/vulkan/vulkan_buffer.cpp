#include "graphics/vulkan/vulkan_buffer.hpp"

#include <stdexcept>

#include "graphics/vulkan/device.hpp"

namespace fluid::graphics {

VulkanBuffer::~VulkanBuffer() {
    cleanup();
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
: buffer(other.buffer),
  bufferMemory(other.bufferMemory),
  size(other.size),
  usageFlags(other.usageFlags),
  memoryProperties(other.memoryProperties),
  _device(other._device),
  _mapped(other._mapped) {
    other.buffer = VK_NULL_HANDLE;
    other.bufferMemory = VK_NULL_HANDLE;
    other.size = 0;
    other.usageFlags = 0;
    other.memoryProperties = 0;
    other._device = nullptr;
    other._mapped = nullptr;
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept {
    if (this == &other) return *this;

    cleanup();

    buffer = other.buffer;
    bufferMemory = other.bufferMemory;
    size = other.size;
    usageFlags = other.usageFlags;
    memoryProperties = other.memoryProperties;
    _device = other._device;
    _mapped = other._mapped;

    other.buffer = VK_NULL_HANDLE;
    other.bufferMemory = VK_NULL_HANDLE;
    other.size = 0;
    other.usageFlags = 0;
    other.memoryProperties = 0;
    other._device = nullptr;
    other._mapped = nullptr;

    return *this;
}

void VulkanBuffer::createBuffer(
    VkDeviceSize psize,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    Device& p_device
) {
    if (psize == 0) {
        throw std::runtime_error("VulkanBuffer::createBuffer() -> buffer size must be non-zero");
    }

    cleanup();
    _device = &p_device;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = psize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(p_device.getDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        _device = nullptr;
        throw std::runtime_error("VulkanBuffer::createBuffer() -> failed to create buffer");
    }

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(p_device.getDevice(), buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = p_device.findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(p_device.getDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        cleanup();
        throw std::runtime_error("VulkanBuffer::createBuffer() -> failed to allocate buffer memory");
    }

    if (vkBindBufferMemory(p_device.getDevice(), buffer, bufferMemory, 0) != VK_SUCCESS) {
        cleanup();
        throw std::runtime_error("VulkanBuffer::createBuffer() -> failed to bind buffer memory");
    }

    size = psize;
    usageFlags = usage;
    memoryProperties = properties;
}

void* VulkanBuffer::map(VkDeviceSize offset, VkDeviceSize mappedSize) {
    if ((memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
        throw std::runtime_error("VulkanBuffer::map() -> buffer memory is not host-visible");
    }
    if (_device == nullptr || bufferMemory == VK_NULL_HANDLE) {
        throw std::runtime_error("VulkanBuffer::map() -> buffer has not been created");
    }
    if (_mapped != nullptr) {
        return _mapped;
    }

    if (vkMapMemory(_device->getDevice(), bufferMemory, offset, mappedSize, 0, &_mapped) != VK_SUCCESS) {
        throw std::runtime_error("VulkanBuffer::map() -> failed to map buffer memory");
    }

    return _mapped;
}

void VulkanBuffer::unmap() {
    if (_mapped != nullptr && _device != nullptr && _device->getDevice() != VK_NULL_HANDLE) {
        vkUnmapMemory(_device->getDevice(), bufferMemory);
    }
    _mapped = nullptr;
}

void VulkanBuffer::cleanup() {
    if (_device != nullptr && _device->getDevice() != VK_NULL_HANDLE) {
        unmap();

        if (buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(_device->getDevice(), buffer, nullptr);
        }

        if (bufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(_device->getDevice(), bufferMemory, nullptr);
        }
    }

    buffer = VK_NULL_HANDLE;
    bufferMemory = VK_NULL_HANDLE;
    size = 0;
    usageFlags = 0;
    memoryProperties = 0;
    _device = nullptr;
    _mapped = nullptr;
}

} // namespace fluid::graphics
