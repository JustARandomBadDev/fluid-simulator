#include "app/application.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <utility>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace fluid {

Application::Application(ApplicationConfig config)
    : _config(std::move(config)),
      _use_cuda_interop(
          _config.simulation.backend == SimulationBackend::Cuda &&
          _config.graphics.enableCudaInterop
      ),
      _camera(_config.camera.position, 45.0f, 16.0f / 9.0f, 0.1f, 20.0f),
      _fluid_simulator(_config.simulation),
      _particle_vertices(_config.simulation.particleCapacity) {}

Application::~Application() {
    cleanup();
}

void Application::run(bool smokeTest) {
    createWindow();
    initializeGraphics();
    initializeSimulation();
    mainLoop(smokeTest);
    cleanup();
}

void Application::createWindow() {
    glfwSetErrorCallback([](int, const char* description) {
        (void)description;
    });

    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("failed to initialize GLFW");
    }
    _glfw_initialized = true;

    if (glfwVulkanSupported() != GLFW_TRUE) {
        throw std::runtime_error("GLFW could not find a Vulkan loader and ICD");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(
        GLFW_RESIZABLE,
        _config.window.resizable ? GLFW_TRUE : GLFW_FALSE
    );
    _window = glfwCreateWindow(
        _config.window.width,
        _config.window.height,
        _config.window.title,
        nullptr,
        nullptr
    );

    if (_window == nullptr) {
        throw std::runtime_error("failed to create the GLFW window");
    }
}

void Application::initializeGraphics() {
    uint32_t extensionCount = 0;
    const char** glfwExtensions =
        glfwGetRequiredInstanceExtensions(&extensionCount);
    if (glfwExtensions == nullptr || extensionCount == 0) {
        throw std::runtime_error(
            "GLFW did not provide the required Vulkan instance extensions"
        );
    }

    graphics::VulkanHostConfig hostConfig;
    for (uint32_t index = 0; index < extensionCount; ++index) {
        hostConfig.requiredInstanceExtensions.push_back(
            glfwExtensions[index]
        );
    }

    if (_use_cuda_interop) {
        hostConfig.requiredInstanceExtensions.push_back(
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
        );
        hostConfig.requiredInstanceExtensions.push_back(
            VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME
        );
        hostConfig.requiredInstanceExtensions.push_back(
            VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME
        );
    }

    hostConfig.createSurface = [this](
                                   VkInstance instance,
                                   VkSurfaceKHR& surface
                               ) {
        return glfwCreateWindowSurface(instance, _window, nullptr, &surface);
    };
    hostConfig.getFramebufferExtent = [this]() {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(_window, &width, &height);
        return VkExtent2D{static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)};
    };

    _config.graphics.vulkanHost = std::move(hostConfig);

    _config.graphics.graphicsResources.particleVertexShader =
        std::filesystem::path(FLUID_SHADER_DIR) / "particle.vert.spv";
    _config.graphics.graphicsResources.particleFragmentShader =
        std::filesystem::path(FLUID_SHADER_DIR) / "particle.frag.spv";

#ifdef FLUID_ENABLE_VALIDATION
    _config.graphics.enableValidationLayers = true;
#else
    _config.graphics.enableValidationLayers = false;
#endif

    _config.graphics.particleCapacity = _config.simulation.particleCapacity;
    _config.graphics.enableCudaInterop = _use_cuda_interop;

    _graphics.init(_config.graphics);
}

void Application::initializeSimulation() {
    _fluid_simulator.init(_config.simulation);

    if (!_use_cuda_interop) return;

    const std::size_t slotCount = _graphics.getCudaInteropSlotCount();
    _fluid_simulator.configureCudaInterop(slotCount);

    for (std::size_t slot = 0; slot < slotCount; slot++) {
        const uint32_t frameIndex = static_cast<uint32_t>(slot);
        const graphics::CudaInteropBufferExport buffer =
            _graphics.exportCudaInteropBuffer(frameIndex);
        _fluid_simulator.importCudaInteropBuffer(
            buffer.fd,
            buffer.allocationSize,
            buffer.bufferSize,
            slot
        );
        _fluid_simulator.importCudaInteropSemaphore(
            _graphics.exportCudaInteropSemaphoreFd(frameIndex),
            slot
        );
    }
}

void Application::mainLoop(bool smokeTest) {
    uint32_t renderedFrames = 0;

    while (glfwWindowShouldClose(_window) == GLFW_FALSE) {
        glfwPollEvents();

        int width = 0;
        int height = 0;

        glfwGetFramebufferSize(_window, &width, &height);

        if (width == 0 || height == 0) {
            glfwWaitEvents();
            continue;
        }

        if (_use_cuda_interop && !_graphics.prepareCudaInteropFrame()) {
            continue;
        }

        _timer.update();

        update(_timer.getDeltaTime());

        _camera.updateProjection(_graphics.getAspectRatio());
        if (_use_cuda_interop) {
            _graphics.renderCudaInterop(
                _camera,
                _active_particle_count,
                _cuda_interop_sync.cudaReadyValue,
                _cuda_interop_sync.vulkanDoneValue
            );
        } else {
            _graphics.render(_camera);
        }

        if (smokeTest) {
            ++renderedFrames;
            if (renderedFrames == 30) {
                glfwSetWindowSize(_window, 960, 540);
            } else if (renderedFrames == 90) {
                glfwSetWindowShouldClose(_window, GLFW_TRUE);
            }
        }
    }
}

void Application::update(float dt) {
    const auto& particles = _fluid_simulator.update(dt);
    _active_particle_count = static_cast<uint32_t>(particles.count);

    if (_use_cuda_interop) {
        _cuda_interop_sync =
            _fluid_simulator.writeCudaRenderBuffer(_graphics.getCurrentFrame());
        return;
    }

    for (std::size_t i = 0; i < particles.count; i++) {
        _particle_vertices[i].x = particles.position_x[i];
        _particle_vertices[i].y = particles.position_y[i];
        _particle_vertices[i].z = particles.position_z[i];
    }

    _graphics.updateParticles({_particle_vertices.data(), particles.count});
}

void Application::cleanup() {
    _graphics.waitIdle();
    _fluid_simulator.cleanupCudaInterop();
    _graphics.cleanup();

    if (_window != nullptr) {
        glfwDestroyWindow(_window);
        _window = nullptr;
    }

    if (_glfw_initialized) {
        glfwTerminate();
        _glfw_initialized = false;
    }
}

} // namespace fluid
