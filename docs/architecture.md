# Architecture

[Back to the README](../README.md)

Fluid Simulator currently consists of a GLFW application, a backend-neutral
simulation facade with one CPU implementation, and a Vulkan particle renderer.
The code is separated by responsibility rather than built as independent
libraries: CMake compiles every `src/*.cpp` file into one `fluid-simulator`
executable.

## Components

| Area            | Location                                         | Responsibility                                                                                      |
| --------------- | ------------------------------------------------ | --------------------------------------------------------------------------------------------------- |
| Application     | `include/app`, `src/app`                         | Window ownership, initialization, frame loop, and transfer of simulated positions to rendering data |
| Core            | `include/core`, `src/core`                       | Camera matrices and frame delta timing                                                              |
| Simulation      | `include/simulation`, `src/simulation`           | Particle ownership, solver interface, SPH parameters/constants, and spatial grid                    |
| CPU backend     | `include/backends/cpu`, `src/backends/cpu`       | Scalar SPH implementation and OpenMP work sharing                                                   |
| Vulkan graphics | `include/graphics/vulkan`, `src/graphics/vulkan` | Instance/device setup, swapchain lifecycle, pipeline, buffers, command recording, and presentation  |
| Shaders         | `shaders`                                        | Particle vertex transform and circular point-sprite coloring                                        |

## Runtime flow

Initialization follows this order:

1. `Application` initializes GLFW and creates a resizable 1280 × 720 window
   without an OpenGL context.
2. GLFW supplies the Vulkan instance extensions, surface callback, and current
   framebuffer extent to `GraphicsRuntime`.
3. The graphics lifecycle creates the Vulkan instance and optional debug
   messenger, the GLFW surface, a suitable physical and logical device, the
   swapchain, render pass, graphics pipeline, command pool, depth resources,
   framebuffers, command buffers, and synchronization objects.
4. One persistently mapped particle vertex buffer is created for each frame in
   flight. The default is two frames.
5. `ParticleSystem` allocates its arrays at the 100,000-particle capacity and
   inserts 20,000 particles. `CpuScalarSolver` allocates its working arrays and
   Uniform Grid.

Each visible frame then performs:

1. GLFW event polling and framebuffer-size handling.
2. A CPU simulation step using the measured frame delta, capped at 1 ms.
3. A copy of active particle positions from `ParticleData` into the
   application's `ParticleVertex` array.
4. A wait for the current frame fence, followed by a `memcpy` into that frame's
   host-visible, host-coherent Vulkan vertex buffer.
5. Command-buffer recording with the current view-projection matrix as a push
   constant.
6. A point-list draw, queue submission, and presentation.

The swapchain is rebuilt when acquisition or presentation reports an
out-of-date/suboptimal swapchain, or when the framebuffer extent changes.
Rendering pauses while the framebuffer has zero width or height, such as when
the window is minimized.

## Simulation/backend boundary

`FluidSimulator` owns a `ParticleSystem` and a `std::unique_ptr<Solver>`.
`Solver` exposes two operations:

- initialize a backend for a simulation box;
- advance a mutable `ParticleData` instance by one step.

`CpuScalarSolver` is the only concrete implementation. There are no CUDA
sources, CUDA CMake language settings, device-resident particle containers, or
Vulkan external-memory primitives in the current repository. A future CUDA
backend will therefore require both a new solver implementation and an
explicit data-ownership/interoperability design.

The simulation details are covered in [Simulation](simulation.md).

## Vulkan rendering path

The renderer requests Vulkan API 1.0 and selects a physical device only when it
provides:

- graphics and presentation queues;
- `VK_KHR_swapchain`;
- usable surface formats and presentation modes;
- the `largePoints` device feature.

The pipeline uses a color attachment and a depth attachment, dynamic viewport
and scissor state, no culling, and point-list input. A push constant carries
the camera view-projection matrix. Each vertex contains only a `vec3` position.

The vertex shader transforms each point and assigns a five-pixel point size.
The fragment shader discards fragments outside a circle and writes a fixed blue
color. There is currently no lighting, density-based shading, transparency, or
surface reconstruction.

`GraphicsRuntime` keeps window-system details behind `VulkanHostConfig`:
required instance extensions, surface creation, and framebuffer extent are
callbacks supplied by the GLFW application. The renderer itself does not call
GLFW.

## Resource ownership

Vulkan resources are wrapped by focused classes rather than one monolithic
renderer:

- `Instance` owns the instance, surface, and optional debug messenger.
- `Device` owns the physical/logical device, queues, and depth image resources.
- `Swapchain` owns swapchain images, views, and framebuffers.
- `GraphicPipeline` owns the render pass, pipeline layout, and graphics
  pipeline.
- `Renderer` owns the command pool, command buffers, semaphores, and fences.
- `ParticleBuffer` owns the per-frame `VulkanBuffer` objects and their mapped
  pointers.
- `CommandRecorder` records a draw for a selected swapchain image.
- `FrameRenderer` coordinates acquire, submit, and present.
- `GraphicsRuntimeLifecycle` creates and recreates dependent resources in the
  required order.

## Current architectural constraints

- The application selects `CpuScalarSolver` directly; there is no runtime
  backend selection.
- Simulation storage uses host `std::vector` allocations and is directly
  mutable through `ParticleData`.
- Rendering needs an additional AoS position array because simulation data is
  stored as SoA.
- The Uniform Grid rebuild is serial; only the particle physics passes use
  OpenMP.
- `GridCell` remains in the source tree but is not used by the active flat-array
  Uniform Grid implementation.
- The project produces one executable and does not expose installable library
  targets.
