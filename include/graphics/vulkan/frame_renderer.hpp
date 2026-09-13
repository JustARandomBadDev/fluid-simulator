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

enum class FrameRenderStatus {
    Rendered,
    NeedsRecreate
};

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
        uint32_t p_vertex_count
    );

private:
    Device& _device;
    Renderer& _renderer;
    Swapchain& _swapchain;
    CommandRecorder& _command_recorder;

    VkResult acquireFrameImage(uint32_t& p_image_index);
    void submitFrame(uint32_t p_image_index);
    VkResult presentFrame(uint32_t p_image_index);
};

} // namespace fluid::graphics

#endif
