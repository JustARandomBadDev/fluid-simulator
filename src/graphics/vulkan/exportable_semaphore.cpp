#include "graphics/vulkan/exportable_semaphore.hpp"

#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace fluid::graphics {

ExportableSemaphore::ExportableSemaphore(ExportableSemaphore&& p_other) noexcept
    : _device(p_other._device), _semaphore(p_other._semaphore) {
    p_other._device = VK_NULL_HANDLE;
    p_other._semaphore = VK_NULL_HANDLE;
}

ExportableSemaphore&
ExportableSemaphore::operator=(ExportableSemaphore&& p_other) noexcept {
    if (this != &p_other) {
        cleanup();

        _device = p_other._device;
        _semaphore = p_other._semaphore;

        p_other._device = VK_NULL_HANDLE;
        p_other._semaphore = VK_NULL_HANDLE;
    }

    return *this;
}

ExportableSemaphore::~ExportableSemaphore() {
    cleanup();
}

VkSemaphore
ExportableSemaphore::createExternalTimelineSemaphore(VkDevice p_device) {
    cleanup();

    _device = p_device;

    VkExportSemaphoreCreateInfo export_info{};
    export_info.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
    export_info.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;

    VkSemaphoreTypeCreateInfo timeline_info{};
    timeline_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    timeline_info.pNext = &export_info;
    timeline_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    timeline_info.initialValue = 0;

    VkSemaphoreCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    create_info.pNext = &timeline_info;

    if (vkCreateSemaphore(p_device, &create_info, nullptr, &_semaphore) !=
        VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create external timeline semaphore"
        );
    }

    return _semaphore;
}

int ExportableSemaphore::exportSemaphoreFd() const {
    auto vkGetSemaphoreFdKHR = reinterpret_cast<PFN_vkGetSemaphoreFdKHR>(
        vkGetDeviceProcAddr(_device, "vkGetSemaphoreFdKHR")
    );

    if (vkGetSemaphoreFdKHR == nullptr) {
        throw std::runtime_error("vkGetSemaphoreFdKHR is not available");
    }

    VkSemaphoreGetFdInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
    info.semaphore = _semaphore;
    info.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;

    int fd = -1;

    if (vkGetSemaphoreFdKHR(_device, &info, &fd) != VK_SUCCESS) {
        throw std::runtime_error("Failed to export semaphore FD");
    }

    return fd;
}

void ExportableSemaphore::cleanup() {
    if (_device != VK_NULL_HANDLE && _semaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(_device, _semaphore, nullptr);
    }
    _semaphore = VK_NULL_HANDLE;
}

} // namespace fluid::graphics
