#ifndef FLUID_CONFIG_HPP
#define FLUID_CONFIG_HPP

#include <cstddef>
#include <cstdint>

#include <filesystem>
#include <functional>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "simulation/sph_parameters.hpp"

namespace fluid {

namespace graphics {

struct VulkanHostConfig {
    std::vector<std::string> requiredInstanceExtensions;
    std::function<VkResult(VkInstance, VkSurfaceKHR&)> createSurface;
    std::function<VkExtent2D()> getFramebufferExtent;
};

struct GraphicsResourceConfig {
    std::filesystem::path particleVertexShader;
    std::filesystem::path particleFragmentShader;
};

struct GraphicsRuntimeConfig {
    VulkanHostConfig vulkanHost;
    glm::vec4 clearColor = {0.015f, 0.025f, 0.055f, 1.0f};

    std::size_t particleCapacity{};

    uint32_t framesInFlight = 2;

    bool enableValidationLayers = true;
    bool enableCudaInterop = true;

    GraphicsResourceConfig graphicsResources;
};

} // namespace graphics

enum class SimulationBackend { CpuScalar, Cuda };

struct WindowConfig {
    int width = 1280;
    int height = 720;
    const char* title = "Fluid Simulator - Vulkan particles";
    bool resizable = true;
};

struct CameraConfig {
    glm::vec3 position{2.0f, 2.0f, 9.0f};
};

struct ParticleSpawnConfig {
    std::size_t count = 100'000;

    glm::vec3 position{1.f, 0.5f, 1.f};
    glm::vec3 dimensions{2.0f, 5.5f, 2.0f};

    glm::vec3 initialVelocity{0.0f};
};

struct SimulationBoxConfig {
    glm::vec3 position{0.0f};
    glm::vec3 dimensions{4.0f, 6.0f, 4.0f};
};

struct CudaConfig {
    std::size_t blockSize = 256;
};

struct SimulationConfig {
    SimulationBackend backend = SimulationBackend::Cuda;

    std::size_t particleCapacity = 100'000;

    SimulationBoxConfig box;
    ParticleSpawnConfig spawn;

    glm::vec3 gravity{0.0f, -9.81f, 0.0f};
    float maximumTimeStep = 0.001f;

    simulation::SphParameters sph;
    CudaConfig cuda;
};

struct ApplicationConfig {
    WindowConfig window;
    CameraConfig camera;
    SimulationConfig simulation;
    graphics::GraphicsRuntimeConfig graphics;
};

} // namespace fluid

#endif
