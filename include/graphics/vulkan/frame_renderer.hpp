#ifndef FLUID_GRAPHICS_VULKAN_FRAME_RENDERER_HPP
#define FLUID_GRAPHICS_VULKAN_FRAME_RENDERER_HPP

#include <cstdint>

#include <glm/vec4.hpp>
#include <vulkan/vulkan.h>

namespace fluid::core {
class Camera;
}

namespace fluid::graphics {

class CommandRecorder;
class Device;
class Renderer;
class Swapchain;
enum class FrameRenderStatus { Rendered, NeedsRecreate };

class FrameRenderer {
  public:
    FrameRenderer(
        Device& p_device,
        Renderer& p_renderer,
        Swapchain& p_swapchain,
        CommandRecorder& p_command_recorder
    );

    FrameRenderStatus render(
        const core::Camera& camera,
        const glm::vec4& p_clear_color,
        VkBuffer p_particle_vertex_buffer,
        uint32_t p_vertex_count,
        VkSemaphore p_cuda_interop_semaphore = VK_NULL_HANDLE,
        uint64_t p_cuda_ready_value = 0,
        uint64_t p_vulkan_done_value = 0
    );

    bool prepareFrame();
    void waitForCurrentFrame();

  private:
    Device& _device;
    Renderer& _renderer;
    Swapchain& _swapchain;
    CommandRecorder& _command_recorder;

    uint32_t _prepared_image_index = 0;
    VkResult _prepared_acquire_result = VK_SUCCESS;
    bool _frame_prepared = false;

    VkResult acquireFrameImage(uint32_t& p_image_index);
    void submitFrame(
        uint32_t p_image_index,
        VkSemaphore p_cuda_interop_semaphore,
        uint64_t p_cuda_ready_value,
        uint64_t p_vulkan_done_value
    );
    VkResult presentFrame(uint32_t p_image_index);
};

} // namespace fluid::graphics

#endif
