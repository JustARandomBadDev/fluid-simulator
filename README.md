# fluid-simulator

3D fluid simulator in C++ with CPU multithreading, SIMD, CUDA acceleration,
Vulkan rendering and neural-network-based simulation.

The current milestone is a standalone Vulkan renderer. It opens a GLFW window
and draws a small static 3D particle cloud while handling window resize and
swapchain recreation.

## Build

The project currently requires CMake 3.24+, a C++20 compiler, Vulkan, GLFW,
GLM, and `glslc`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## Run

```sh
./build/fluid-simulator
```

For an automated launch, resize, and clean-shutdown check:

```sh
./build/fluid-simulator --smoke-test
```
