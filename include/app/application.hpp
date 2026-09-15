#ifndef FLUID_APP_APPLICATION_HPP
#define FLUID_APP_APPLICATION_HPP

#include <array>

#include "core/timer.hpp"
#include "core/camera.hpp"
#include "graphics/vulkan/graphics_runtime.hpp"
#include "graphics/vulkan/particle_vertex.hpp"
#include "simulation/fluid_simulator.hpp"

struct GLFWwindow;

namespace fluid {

class Application {
public:
    Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    
    ~Application();

    void run(bool smokeTest = false);

private:
    GLFWwindow* _window = nullptr;
    graphics::GraphicsRuntime _graphics;
    core::Camera _camera{{0.0f, 0.5f, 2.0f}, 45.0f, 16.0f / 9.0f, 0.1f, 20.0f};
    bool _glfw_initialized = false;

    simulation::FluidSimulator _fluid_simulator;
    std::array<graphics::ParticleVertex, simulation::ParticleSystem::MAX_PARTICLES> _particle_vertices {};

    core::Timer _timer;

    void createWindow();
    void initializeGraphics();
    void initializeSimulation();
    void mainLoop(bool smokeTest);
    void update(float dt);
    void cleanup();
};

} // namespace fluid

#endif
