#ifndef FLUID_GRAPHICS_VULKAN_RUNTIME_LIFECYCLE_HPP
#define FLUID_GRAPHICS_VULKAN_RUNTIME_LIFECYCLE_HPP

#include <cstdint>
#include <vector>

#include <vulkan/vulkan.h>

#include "graphics/vulkan/graphics_config.hpp"
#include "graphics/vulkan/particle_vertex.hpp"

namespace fluid::graphics {

class Device;
class GraphicPipeline;
class Instance;
class Renderer;
class Swapchain;
class VulkanBuffer;

class GraphicsRuntimeLifecycle {
public:
    GraphicsRuntimeLifecycle(
        Instance& p_instance,
        Device& p_device,
        Renderer& p_renderer,
        Swapchain& p_swapchain,
        GraphicPipeline& p_graphic_pipeline,
        VulkanBuffer& p_particle_vertex_buffer
    );

    void initialize(
        const VulkanHostConfig& p_host_config,
        const GraphicsResourceConfig& p_resources,
        const std::vector<ParticleVertex>& p_particles,
        uint32_t p_frames_in_flight,
        bool p_enable_validation_layers,
        VkExtent2D p_framebuffer_extent
    );

    void recreateSwapchain(VkExtent2D p_framebuffer_extent);
    void cleanupSwapchainDependentResources();

private:
    Instance& _instance;
    Device& _device;
    Renderer& _renderer;
    Swapchain& _swapchain;
    GraphicPipeline& _graphic_pipeline;
    VulkanBuffer& _particle_vertex_buffer;
    GraphicsResourceConfig _resources;
    uint32_t _frames_in_flight = 0;

    void createSwapchainResources(VkExtent2D p_framebuffer_extent, uint32_t p_frames_in_flight);
    void createSwapchainRenderTargets();
    void createPipelineResources();
    void createFrameResources(uint32_t p_frames_in_flight);
    void createPersistentResources(const std::vector<ParticleVertex>& p_particles);
};

} // namespace fluid::graphics

#endif
