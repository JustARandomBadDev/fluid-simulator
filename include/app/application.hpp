#ifndef FLUID_APP_APPLICATION_HPP
#define FLUID_APP_APPLICATION_HPP

#include <cstddef>
#include <cstdint>
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
    std::vector<double> runFrameBenchmark(
        std::size_t p_warmup_frames,
        std::size_t p_measured_frames,
        float p_delta_time
    );

  private:
    ApplicationConfig _config;
    bool _use_cuda_interop = false;
    GLFWwindow* _window = nullptr;
    graphics::GraphicsRuntime _graphics;
    core::Camera _camera;
    bool _glfw_initialized = false;

    simulation::FluidSimulator _fluid_simulator;
    std::vector<graphics::ParticleVertex> _particle_vertices;
    simulation::CudaInteropSyncValues _cuda_interop_sync;
    uint32_t _active_particle_count = 0;

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
