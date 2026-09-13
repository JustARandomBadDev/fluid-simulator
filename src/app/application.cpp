#include "app/application.hpp"

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <utility>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace fluid {
namespace {

constexpr int kInitialWindowWidth = 1280;
constexpr int kInitialWindowHeight = 720;
constexpr const char* kWindowTitle = "Fluid Simulator - Vulkan particles";

} // namespace

Application::~Application() {
    cleanup();
}

void Application::run(bool smokeTest) {
    createWindow();
    initializeGraphics();
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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    _window = glfwCreateWindow(
        kInitialWindowWidth,
        kInitialWindowHeight,
        kWindowTitle,
        nullptr,
        nullptr
    );

    if (_window == nullptr) {
        throw std::runtime_error("failed to create the GLFW window");
    }
}

void Application::initializeGraphics() {
    uint32_t extensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    if (glfwExtensions == nullptr || extensionCount == 0) {
        throw std::runtime_error("GLFW did not provide the required Vulkan instance extensions");
    }

    graphics::VulkanHostConfig hostConfig;
    hostConfig.requiredInstanceExtensions.reserve(extensionCount);
    for (uint32_t index = 0; index < extensionCount; ++index) {
        hostConfig.requiredInstanceExtensions.emplace_back(glfwExtensions[index]);
    }

    hostConfig.createSurface = [this](VkInstance instance, VkSurfaceKHR& surface) {
        return glfwCreateWindowSurface(instance, _window, nullptr, &surface);
    };
    hostConfig.getFramebufferExtent = [this]() {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(_window, &width, &height);
        return VkExtent2D{
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
    };

    graphics::GraphicsRuntimeConfig config;
    config.vulkanHost = std::move(hostConfig);
    config.graphicsResources.particleVertexShader =
        std::filesystem::path(FLUID_SHADER_DIR) / "particle.vert.spv";
    config.graphicsResources.particleFragmentShader =
        std::filesystem::path(FLUID_SHADER_DIR) / "particle.frag.spv";
#ifdef FLUID_ENABLE_VALIDATION
    config.enableValidationLayers = true;
#else
    config.enableValidationLayers = false;
#endif

    _graphics.init(config);
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

        _camera.updateProjection(_graphics.getAspectRatio());
        _graphics.render(_camera);

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

void Application::cleanup() {
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
