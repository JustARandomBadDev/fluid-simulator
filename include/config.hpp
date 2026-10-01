#ifndef FLUID_CONFIG_HPP
#define FLUID_CONFIG_HPP

#include <cstddef>

#include <glm/vec3.hpp>

#include "simulation/sph_parameters.hpp"

namespace fluid {

enum class SimulationBackend { CpuScalar, Cuda };

struct WindowConfig {
    int width = 1280;
    int height = 720;
    const char* title = "Fluid Simulator - Vulkan particles";
    bool resizable = true;
};

struct CameraConfig {
    glm::vec3 position{1.0f, 1.0f, 5.0f};
};

struct ParticleSpawnConfig {
    std::size_t count = 20'000;

    glm::vec3 position{0.5f, 0.5f, 0.5f};
    glm::vec3 dimensions{1.0f, 2.5f, 1.0f};

    glm::vec3 initialVelocity{0.0f};
};

struct SimulationBoxConfig {
    glm::vec3 position{0.0f};
    glm::vec3 dimensions{2.0f, 20.0f, 2.0f};
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
};

} // namespace fluid

#endif
