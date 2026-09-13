#ifndef FLUID_GRAPHICS_VULKAN_GRAPHICS_RUNTIME_HPP
#define FLUID_GRAPHICS_VULKAN_GRAPHICS_RUNTIME_HPP

#include <vector>

#include <vulkan/vulkan.h>

#include "graphics/vulkan/command_recorder.hpp"
#include "graphics/vulkan/device.hpp"
#include "graphics/vulkan/frame_renderer.hpp"
#include "graphics/vulkan/graphic_pipeline.hpp"
#include "graphics/vulkan/graphics_config.hpp"
#include "graphics/vulkan/instance.hpp"
#include "graphics/vulkan/particle_vertex.hpp"
#include "graphics/vulkan/renderer.hpp"
#include "graphics/vulkan/runtime_lifecycle.hpp"
#include "graphics/vulkan/swapchain.hpp"
#include "graphics/vulkan/vulkan_buffer.hpp"

namespace fluid::core {
class Camera;
}

namespace fluid::graphics {

class GraphicsRuntime {
public:
    GraphicsRuntime();
    ~GraphicsRuntime();

    GraphicsRuntime(const GraphicsRuntime&) = delete;
    GraphicsRuntime& operator=(const GraphicsRuntime&) = delete;

    void init(const GraphicsRuntimeConfig& config);
    void render(const core::Camera& camera);
    void cleanup();

    float getAspectRatio() const;

private:
    Instance instance;
    Device device;
    VulkanBuffer particleVertexBuffer;
    Renderer renderer;
    Swapchain swapchain;
    GraphicPipeline graphicPipeline;
    GraphicsRuntimeLifecycle runtimeLifecycle;
    CommandRecorder frameCommandRecorder;
    FrameRenderer frameRenderer;

    glm::vec4 _clear_color = {0.015f, 0.025f, 0.055f, 1.0f};
    VulkanHostConfig _host_config;
    bool _swapchain_needs_recreate = false;
    bool _initialized = false;
    uint32_t _particle_count = 0;

    bool recreateSwapchain();
    VkExtent2D getFramebufferExtent() const;
    bool ensureSwapchainReady();
    static std::vector<ParticleVertex> makeValidationParticles();
};

} // namespace fluid::graphics

#endif
