#ifndef FLUID_GRAPHICS_VULKAN_COMMAND_RECORDER_HPP
#define FLUID_GRAPHICS_VULKAN_COMMAND_RECORDER_HPP

#include <cstdint>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace fluid::graphics {

class GraphicPipeline;
class Renderer;
class Swapchain;
class VulkanBuffer;

class CommandRecorder {
public:
    CommandRecorder(
        Renderer& p_renderer,
        Swapchain& p_swapchain,
        GraphicPipeline& p_graphic_pipeline
    );

    void record(
        uint32_t p_image_index,
        const glm::vec4& p_clear_color,
        const glm::mat4& p_view_projection,
        const VulkanBuffer& p_particle_vertex_buffer,
        uint32_t p_vertex_count
    );

private:
    Renderer& _renderer;
    Swapchain& _swapchain;
    GraphicPipeline& _graphic_pipeline;
};

} // namespace fluid::graphics

#endif
