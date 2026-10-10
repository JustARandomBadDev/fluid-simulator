#ifndef FLUID_GRAPHICS_VULKAN_EXPORTABLE_SEMAPHORE_HPP
#define FLUID_GRAPHICS_VULKAN_EXPORTABLE_SEMAPHORE_HPP

#include <vulkan/vulkan_core.h>

namespace fluid::graphics {

class ExportableSemaphore {
  public:
    ExportableSemaphore() = default;
    ~ExportableSemaphore();

    ExportableSemaphore(const ExportableSemaphore&) = delete;
    ExportableSemaphore& operator=(const ExportableSemaphore&) = delete;

    ExportableSemaphore(ExportableSemaphore&& p_other) noexcept;
    ExportableSemaphore& operator=(ExportableSemaphore&& p_other) noexcept;

    VkSemaphore createExternalTimelineSemaphore(VkDevice p_device);
    int exportSemaphoreFd() const;
    void cleanup();

    VkSemaphore getSemaphore() const {
        return _semaphore;
    }

  private:
    VkDevice _device = VK_NULL_HANDLE;
    VkSemaphore _semaphore = VK_NULL_HANDLE;
};

} // namespace fluid::graphics

#endif
