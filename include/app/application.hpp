#ifndef FLUID_APP_APPLICATION_HPP
#define FLUID_APP_APPLICATION_HPP

#include <array>

#include "core/camera.hpp"
#include "graphics/vulkan/graphics_runtime.hpp"
#include "graphics/vulkan/particle_vertex.hpp"
#include "simulation/particle_system.hpp"

struct GLFWwindow;

namespace fluid {

class Application {
public:
    Application() = default;
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void run(bool smokeTest = false);

private:
    GLFWwindow* _window = nullptr;
    graphics::GraphicsRuntime _graphics;
    core::Camera _camera{{0.0f, 0.0f, 4.0f}, 45.0f, 16.0f / 9.0f, 0.1f, 20.0f};
    bool _glfw_initialized = false;

    simulation::ParticleSystem _particle_system;
    std::array<graphics::ParticleVertex, simulation::ParticleSystem::MAX_PARTICLES>
        _particle_vertices{};

    void createWindow();
    void initializeGraphics();
    void initializeSimulation();
    void mainLoop(bool smokeTest);
    void update();
    void cleanup();
};

} // namespace fluid

#endif
