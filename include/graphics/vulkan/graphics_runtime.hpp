#ifndef FLUID_GRAPHICS_VULKAN_GRAPHICS_RUNTIME_HPP
#define FLUID_GRAPHICS_VULKAN_GRAPHICS_RUNTIME_HPP

#include <cstddef>
#include <cstdint>
#include <span>

#include <vulkan/vulkan.h>

#include "config.hpp"
#include "graphics/vulkan/command_recorder.hpp"
#include "graphics/vulkan/device.hpp"
#include "graphics/vulkan/frame_renderer.hpp"
#include "graphics/vulkan/graphic_pipeline.hpp"
#include "graphics/vulkan/instance.hpp"
#include "graphics/vulkan/particle_buffer.hpp"
#include "graphics/vulkan/renderer.hpp"
#include "graphics/vulkan/runtime_lifecycle.hpp"
#include "graphics/vulkan/swapchain.hpp"

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
    void updateParticles(std::span<const ParticleVertex> p_particles);
    void render(const core::Camera& camera);
    bool prepareCudaInteropFrame();
    void renderCudaInterop(
        const core::Camera& p_camera,
        uint32_t p_particle_count,
        uint64_t p_cuda_ready_value,
        uint64_t p_vulkan_done_value
    );
    void waitIdle();
    void cleanup();

    float getAspectRatio() const;
    uint32_t getCurrentFrame() const;
    std::size_t getCudaInteropSlotCount() const;
    CudaInteropBufferExport
    exportCudaInteropBuffer(uint32_t p_frame_index) const;
    int exportCudaInteropSemaphoreFd(uint32_t p_frame_index) const;

  private:
    Instance instance;
    Device device;
    Renderer renderer;
    Swapchain swapchain;
    GraphicPipeline graphicPipeline;
    ParticleBuffer _particle_buffer;
    GraphicsRuntimeLifecycle runtimeLifecycle;
    CommandRecorder frameCommandRecorder;
    FrameRenderer frameRenderer;

    glm::vec4 _clear_color = {0.015f, 0.025f, 0.055f, 1.0f};
    VulkanHostConfig _host_config;
    bool _swapchain_needs_recreate = false;
    bool _initialized = false;
    bool _cuda_interop_enabled = false;

    bool recreateSwapchain();
    VkExtent2D getFramebufferExtent() const;
    bool ensureSwapchainReady();
};

} // namespace fluid::graphics

#endif
