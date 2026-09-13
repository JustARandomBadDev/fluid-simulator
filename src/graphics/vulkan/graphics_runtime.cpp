#include "graphics/vulkan/graphics_runtime.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

#include "core/camera.hpp"

namespace fluid::graphics {
namespace {

void validateRequiredResourcePath(
    const std::filesystem::path& path,
    const char* resource_name
) {
    if (path.empty()) {
        throw std::runtime_error(std::string("missing required resource path: ") + resource_name);
    }

    if (!std::filesystem::exists(path)) {
        throw std::runtime_error(
            std::string("required resource does not exist: ") + resource_name + " -> " + path.string()
        );
    }

    if (!std::filesystem::is_regular_file(path)) {
        throw std::runtime_error(
            std::string("required resource is not a file: ") + resource_name + " -> " + path.string()
        );
    }
}

void validateGraphicsResources(const GraphicsResourceConfig& resources) {
    validateRequiredResourcePath(
        resources.particleVertexShader,
        "graphicsResources.particleVertexShader"
    );
    validateRequiredResourcePath(
        resources.particleFragmentShader,
        "graphicsResources.particleFragmentShader"
    );
}

void validateInitConfig(const GraphicsRuntimeConfig& config) {
    if (!config.vulkanHost.createSurface) {
        throw std::runtime_error("GraphicsRuntime::init() -> vulkanHost.createSurface callback must be set");
    }

    if (!config.vulkanHost.getFramebufferExtent) {
        throw std::runtime_error("GraphicsRuntime::init() -> vulkanHost.getFramebufferExtent callback must be set");
    }

    if (config.vulkanHost.requiredInstanceExtensions.empty()) {
        throw std::runtime_error("GraphicsRuntime::init() -> vulkanHost.requiredInstanceExtensions must not be empty");
    }

    if (config.framesInFlight == 0) {
        throw std::runtime_error("GraphicsRuntime::init() -> framesInFlight must be greater than 0");
    }
}

} // namespace

GraphicsRuntime::GraphicsRuntime()
: runtimeLifecycle(
      instance,
      device,
      renderer,
      swapchain,
      graphicPipeline,
      particleVertexBuffer
  ),
  frameCommandRecorder(renderer, swapchain, graphicPipeline, particleVertexBuffer),
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

void GraphicsRuntime::init(const GraphicsRuntimeConfig& config) {
    validateInitConfig(config);
    validateGraphicsResources(config.graphicsResources);

    _host_config = config.vulkanHost;
    _clear_color = config.clearColor;

    const VkExtent2D framebufferExtent = getFramebufferExtent();
    if (framebufferExtent.width == 0 || framebufferExtent.height == 0) {
        throw std::runtime_error("GraphicsRuntime::init() -> host framebuffer extent must be non-zero during initialization");
    }

    const std::vector<ParticleVertex> particles = makeValidationParticles();
    _particle_count = static_cast<uint32_t>(particles.size());

    try {
        runtimeLifecycle.initialize(
            config.vulkanHost,
            config.graphicsResources,
            particles,
            config.framesInFlight,
            config.enableValidationLayers,
            framebufferExtent
        );
        _initialized = true;
    } catch (...) {
        cleanup();
        throw;
    }
}

void GraphicsRuntime::render(const core::Camera& camera) {
    if (!_initialized) {
        throw std::runtime_error("GraphicsRuntime::render() -> runtime is not initialized");
    }

    if (!ensureSwapchainReady()) return;

    if (frameRenderer.render(camera, _clear_color, _particle_count) ==
        FrameRenderStatus::NeedsRecreate) {
        _swapchain_needs_recreate = true;
        ensureSwapchainReady();
    }
}

void GraphicsRuntime::cleanup() {
    if (device.getDevice() != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device.getDevice());

        runtimeLifecycle.cleanupSwapchainDependentResources();
        swapchain.cleanup(device);
        particleVertexBuffer.cleanup();
        renderer.cleanup(device);
        device.cleanup();
    }
    instance.cleanup();

    _host_config = {};
    _swapchain_needs_recreate = false;
    _initialized = false;
    _particle_count = 0;
}

std::vector<ParticleVertex> GraphicsRuntime::makeValidationParticles() {
    std::vector<ParticleVertex> particles;
    particles.reserve(125);

    constexpr int particlesPerAxis = 5;
    constexpr float spacing = 0.35f;
    constexpr float center = static_cast<float>(particlesPerAxis - 1) * 0.5f;
    for (int z = 0; z < particlesPerAxis; ++z) {
        for (int y = 0; y < particlesPerAxis; ++y) {
            for (int x = 0; x < particlesPerAxis; ++x) {
                particles.push_back({{
                    (static_cast<float>(x) - center) * spacing,
                    (static_cast<float>(y) - center) * spacing,
                    (static_cast<float>(z) - center) * spacing
                }});
            }
        }
    }

    return particles;
}

} // namespace fluid::graphics
