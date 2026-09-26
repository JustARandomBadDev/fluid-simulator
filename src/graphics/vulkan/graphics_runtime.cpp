#include "graphics/vulkan/graphics_runtime.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

#include "core/camera.hpp"

namespace fluid::graphics {
namespace {

void validateRequiredResourcePath(
    const std::filesystem::path &path,
    const char *resource_name
) {
    if (path.empty()) {
        throw std::runtime_error(
            std::string("missing required resource path: ") + resource_name
        );
    }

    if (!std::filesystem::exists(path)) {
        throw std::runtime_error(
            std::string("required resource does not exist: ") + resource_name +
            " -> " + path.string()
        );
    }

    if (!std::filesystem::is_regular_file(path)) {
        throw std::runtime_error(
            std::string("required resource is not a file: ") + resource_name +
            " -> " + path.string()
        );
    }
}

void validateGraphicsResources(const GraphicsResourceConfig &resources) {
    validateRequiredResourcePath(
        resources.particleVertexShader,
        "graphicsResources.particleVertexShader"
    );
    validateRequiredResourcePath(
        resources.particleFragmentShader,
        "graphicsResources.particleFragmentShader"
    );
}

void validateInitConfig(const GraphicsRuntimeConfig &config) {
    if (!config.vulkanHost.createSurface) {
        throw std::runtime_error(
            "GraphicsRuntime::init() -> vulkanHost.createSurface callback must "
            "be set"
        );
    }

    if (!config.vulkanHost.getFramebufferExtent) {
        throw std::runtime_error(
            "GraphicsRuntime::init() -> vulkanHost.getFramebufferExtent "
            "callback must be set"
        );
    }

    if (config.vulkanHost.requiredInstanceExtensions.empty()) {
        throw std::runtime_error(
            "GraphicsRuntime::init() -> vulkanHost.requiredInstanceExtensions "
            "must not be empty"
        );
    }

    if (config.framesInFlight == 0) {
        throw std::runtime_error(
            "GraphicsRuntime::init() -> framesInFlight must be greater than 0"
        );
    }
    if (config.particleCapacity == 0) {
        throw std::runtime_error(
            "GraphicsRuntime::init() -> particleCapacity must be greater than 0"
        );
    }
}

} // namespace

GraphicsRuntime::GraphicsRuntime()
    : _particle_buffer(device),
      runtimeLifecycle(instance, device, renderer, swapchain, graphicPipeline),
      frameCommandRecorder(renderer, swapchain, graphicPipeline),
      frameRenderer(device, renderer, swapchain, frameCommandRecorder) {}

GraphicsRuntime::~GraphicsRuntime() {
    cleanup();
}

VkExtent2D GraphicsRuntime::getFramebufferExtent() const {
    return _host_config.getFramebufferExtent();
}

float GraphicsRuntime::getAspectRatio() const {
    const VkExtent2D framebufferExtent = getFramebufferExtent();
    if (framebufferExtent.width > 0 && framebufferExtent.height > 0) {
        return static_cast<float>(framebufferExtent.width) /
               static_cast<float>(framebufferExtent.height);
    }

    if (swapchain.getImageCount() > 0) {
        return swapchain.getAspectRatio();
    }

    return 1.0f;
}

bool GraphicsRuntime::recreateSwapchain() {
    const VkExtent2D framebufferExtent = getFramebufferExtent();
    if (framebufferExtent.width == 0 || framebufferExtent.height == 0) {
        _swapchain_needs_recreate = true;
        return false;
    }

    runtimeLifecycle.recreateSwapchain(framebufferExtent);
    _swapchain_needs_recreate = false;
    return true;
}

bool GraphicsRuntime::ensureSwapchainReady() {
    const VkExtent2D framebufferExtent = getFramebufferExtent();
    if (framebufferExtent.width == 0 || framebufferExtent.height == 0) {
        _swapchain_needs_recreate = true;
        return false;
    }

    const VkExtent2D swapchainExtent = swapchain.getSwapChainExtent();
    const bool hostExtentChanged =
        swapchain.getImageCount() > 0 &&
        (swapchainExtent.width != framebufferExtent.width ||
            swapchainExtent.height != framebufferExtent.height);

    if (_swapchain_needs_recreate || hostExtentChanged) {
        return recreateSwapchain();
    }

    return true;
}

void GraphicsRuntime::init(const GraphicsRuntimeConfig &config) {
    validateInitConfig(config);
    validateGraphicsResources(config.graphicsResources);

    _host_config = config.vulkanHost;
    _clear_color = config.clearColor;

    const VkExtent2D framebufferExtent = getFramebufferExtent();
    if (framebufferExtent.width == 0 || framebufferExtent.height == 0) {
        throw std::runtime_error(
            "GraphicsRuntime::init() -> host framebuffer extent must be "
            "non-zero during initialization"
        );
    }

    try {
        runtimeLifecycle.initialize(
            config.vulkanHost,
            config.graphicsResources,
            config.framesInFlight,
            config.enableValidationLayers,
            framebufferExtent
        );
        _particle_buffer.initialize(
            config.particleCapacity,
            config.framesInFlight
        );
        _initialized = true;
    } catch (...) {
        cleanup();
        throw;
    }
}

void GraphicsRuntime::updateParticles(
    std::span<const ParticleVertex> p_particles
) {
    if (!_initialized) {
        throw std::runtime_error(
            "GraphicsRuntime::updateParticles() -> runtime is not initialized"
        );
    }

    if (!ensureSwapchainReady()) return;

    frameRenderer.waitForCurrentFrame();
    _particle_buffer.update(renderer.getCurrentFrame(), p_particles);
}

void GraphicsRuntime::render(const core::Camera &camera) {
    if (!_initialized) {
        throw std::runtime_error(
            "GraphicsRuntime::render() -> runtime is not initialized"
        );
    }

    if (!ensureSwapchainReady()) return;

    const uint32_t currentFrame = renderer.getCurrentFrame();
    if (frameRenderer.render(
            camera,
            _clear_color,
            _particle_buffer.buffer(currentFrame),
            _particle_buffer.count(currentFrame)
        ) == FrameRenderStatus::NeedsRecreate) {
        _swapchain_needs_recreate = true;
        ensureSwapchainReady();
    }
}

void GraphicsRuntime::cleanup() {
    if (device.getDevice() != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device.getDevice());

        runtimeLifecycle.cleanupSwapchainDependentResources();
        swapchain.cleanup(device);
        _particle_buffer.cleanup();
        renderer.cleanup(device);
        device.cleanup();
    }
    instance.cleanup();

    _host_config = {};
    _swapchain_needs_recreate = false;
    _initialized = false;
}

} // namespace fluid::graphics
