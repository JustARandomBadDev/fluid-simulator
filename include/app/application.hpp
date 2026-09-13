#ifndef FLUID_APP_APPLICATION_HPP
#define FLUID_APP_APPLICATION_HPP

#include "core/camera.hpp"
#include "graphics/vulkan/graphics_runtime.hpp"

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

    void createWindow();
    void initializeGraphics();
    void mainLoop(bool smokeTest);
    void cleanup();
};

} // namespace fluid

#endif
