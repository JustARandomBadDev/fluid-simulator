#include "graphics/vulkan/runtime_lifecycle.hpp"

#include <cstring>
#include <stdexcept>

#include "graphics/vulkan/device.hpp"
#include "graphics/vulkan/graphic_pipeline.hpp"
#include "graphics/vulkan/instance.hpp"
#include "graphics/vulkan/renderer.hpp"
#include "graphics/vulkan/swapchain.hpp"
#include "graphics/vulkan/vulkan_buffer.hpp"

namespace fluid::graphics {

GraphicsRuntimeLifecycle::GraphicsRuntimeLifecycle(
    Instance& p_instance,
    Device& p_device,
    Renderer& p_renderer,
    Swapchain& p_swapchain,
    GraphicPipeline& p_graphic_pipeline,
    VulkanBuffer& p_particle_vertex_buffer
)
: _instance(p_instance),
  _device(p_device),
  _renderer(p_renderer),
  _swapchain(p_swapchain),
  _graphic_pipeline(p_graphic_pipeline),
  _particle_vertex_buffer(p_particle_vertex_buffer) {}

void GraphicsRuntimeLifecycle::createSwapchainResources(
    VkExtent2D p_framebuffer_extent,
    uint32_t p_frames_in_flight
) {
    if (p_framebuffer_extent.width == 0 || p_framebuffer_extent.height == 0) {
        throw std::runtime_error("GraphicsRuntimeLifecycle::createSwapchainResources() -> host framebuffer extent must be non-zero");
    }

    _swapchain.createSwapChain(
        p_framebuffer_extent,
        _instance,
        _device,
        p_frames_in_flight
    );
    _swapchain.createImageViews(_device);
}

void GraphicsRuntimeLifecycle::createPipelineResources() {
    _graphic_pipeline.createRenderPass(_swapchain, _device);
    _graphic_pipeline.createGraphicsPipeline(
        _resources.particleVertexShader,
        _resources.particleFragmentShader,
        _device
    );
}

void GraphicsRuntimeLifecycle::createSwapchainRenderTargets() {
    _device.recreateDepthResources(_swapchain);
    _swapchain.createFramebuffers(_graphic_pipeline, _device);
}

void GraphicsRuntimeLifecycle::createFrameResources(uint32_t p_frames_in_flight) {
    _renderer.createCommandBuffers(_device, _swapchain.getImageCount());
    _renderer.createSyncObjects(_device, p_frames_in_flight, _swapchain.getImageCount());
}

void GraphicsRuntimeLifecycle::createPersistentResources(
    const std::vector<ParticleVertex>& p_particles
) {
    if (p_particles.empty()) {
        throw std::runtime_error("GraphicsRuntimeLifecycle::createPersistentResources() -> particle list must not be empty");
    }

    const VkDeviceSize bufferSize = static_cast<VkDeviceSize>(
        p_particles.size() * sizeof(ParticleVertex)
    );

    VulkanBuffer stagingBuffer;
    stagingBuffer.createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        _device
    );
    std::memcpy(
        stagingBuffer.map(),
        p_particles.data(),
        static_cast<size_t>(bufferSize)
    );
    stagingBuffer.unmap();

    _particle_vertex_buffer.createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        _device
    );
    _renderer.copyBuffer(
        stagingBuffer,
        _particle_vertex_buffer,
        bufferSize,
        _device
    );
}

void GraphicsRuntimeLifecycle::cleanupSwapchainDependentResources() {
    _renderer.cleanupFrameResources(_device);
    _swapchain.cleanupFramebuffers(_device);
    _device.cleanupDepthResources();
    _graphic_pipeline.cleanup(_device);
}

void GraphicsRuntimeLifecycle::initialize(
    const VulkanHostConfig& p_host_config,
    const GraphicsResourceConfig& p_resources,
    const std::vector<ParticleVertex>& p_particles,
    uint32_t p_frames_in_flight,
    bool p_enable_validation_layers,
    VkExtent2D p_framebuffer_extent
) {
    _resources = p_resources;
    _frames_in_flight = p_frames_in_flight;

    _instance.createInstance(
        p_enable_validation_layers,
        p_host_config.requiredInstanceExtensions
    );
    _instance.setupDebugMessenger();
    _instance.createSurface(p_host_config.createSurface);

    _device.pickPhysicalDevice(_instance, _swapchain);
    _device.createLogicalDevice(_instance);

    createSwapchainResources(p_framebuffer_extent, p_frames_in_flight);
    createPipelineResources();

    _renderer.createCommandPool(_device, _instance);
    createSwapchainRenderTargets();
    createFrameResources(p_frames_in_flight);
    createPersistentResources(p_particles);
}

void GraphicsRuntimeLifecycle::recreateSwapchain(VkExtent2D p_framebuffer_extent) {
    if (vkDeviceWaitIdle(_device.getDevice()) != VK_SUCCESS) {
        throw std::runtime_error("GraphicsRuntimeLifecycle::recreateSwapchain() -> failed to idle device");
    }

    cleanupSwapchainDependentResources();
    _swapchain.cleanup(_device);

    createSwapchainResources(p_framebuffer_extent, _frames_in_flight);
    createPipelineResources();
    createSwapchainRenderTargets();
    createFrameResources(_frames_in_flight);
}

} // namespace fluid::graphics
