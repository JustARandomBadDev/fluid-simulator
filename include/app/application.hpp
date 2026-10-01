#ifndef FLUID_APP_APPLICATION_HPP
#define FLUID_APP_APPLICATION_HPP

#include <vector>

#include "config.hpp"
#include "core/camera.hpp"
#include "core/timer.hpp"
#include "graphics/vulkan/graphics_runtime.hpp"
#include "graphics/vulkan/particle_vertex.hpp"
#include "simulation/fluid_simulator.hpp"

struct GLFWwindow;

namespace fluid {

class Application {
  public:
    explicit Application(ApplicationConfig config = {});
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    ~Application();

    void run(bool smokeTest = false);

  private:
    ApplicationConfig _config;
    GLFWwindow* _window = nullptr;
    graphics::GraphicsRuntime _graphics;
    core::Camera _camera;
    bool _glfw_initialized = false;

    simulation::FluidSimulator _fluid_simulator;
    std::vector<graphics::ParticleVertex> _particle_vertices;

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
