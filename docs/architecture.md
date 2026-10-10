# Architecture

[Back to the README](../README.md)

Fluid Simulator currently consists of a GLFW application, a backend-neutral
simulation facade with CPU and CUDA implementations, and a Vulkan particle
renderer. The code is separated by responsibility rather than built as
independent libraries: CMake compiles the `src/*.cpp` and `src/*.cu` files into
one `fluid-simulator` executable.

## Components

| Area            | Location                                         | Responsibility                                                                                      |
| --------------- | ------------------------------------------------ | --------------------------------------------------------------------------------------------------- |
| Application     | `include/app`, `src/app`                         | Window ownership, initialization, frame loop, and transfer of simulated positions to rendering data |
| Configuration   | `include/config.hpp`                             | Shared defaults for the application, scene, simulation, and backend                                 |
| Benchmark       | `include/benchmark`, `src/benchmark`             | Headless simulation timing and graphical CUDA rendering-path comparison                             |
| Core            | `include/core`, `src/core`                       | Camera matrices and frame delta timing                                                              |
| Simulation      | `include/simulation`, `src/simulation`           | Particle ownership, solver interface, SPH parameters/constants, and spatial grid                    |
| CPU backend     | `include/backends/cpu`, `src/backends/cpu`       | Scalar SPH implementation and OpenMP work sharing                                                   |
| CUDA backend    | `include/backends/cuda`, `src/backends/cuda`     | GPU SPH implementation, CUDA Uniform Grid, and device/interop resource ownership                    |
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
4. `ParticleBuffer` creates one render buffer per frame-in-flight slot. It uses
   persistently mapped `VulkanBuffer` objects normally, or
   `VulkanExportableBuffer` objects and exportable timeline semaphores when
   interoperability is enabled. The default is two frames.
5. `ParticleSystem` allocates its arrays at the configured particle capacity
   and inserts the configured initial particles. `FluidSimulator` constructs
   the selected CPU or CUDA solver; the default CUDA solver allocates device
   arrays for the same capacity and configures the box bounds, SPH parameters,
   and derived constants used by its kernels.

Each visible frame polls events, handles the framebuffer extent, advances the
selected solver, records a point-list draw, submits it, and presents it. The
particle-data path depends on the selected mode:

```text
CPU
  CPU simulation → ParticleVertex packing → mapped VulkanBuffer → draw

CUDA without interop
  CUDA simulation → DeviceToHost → ParticleVertex packing
                  → mapped VulkanBuffer → draw

CUDA with interop
  resident CUDA SoA → CUDA ParticleVertex packing kernel
                    → VulkanExportableBuffer → draw
```

The swapchain is rebuilt when acquisition or presentation reports an
out-of-date/suboptimal swapchain, or when the framebuffer extent changes.
Rendering pauses while the framebuffer has zero width or height, such as when
the window is minimized.

## Simulation/backend boundary

`FluidSimulator` owns a `ParticleSystem` and a `std::unique_ptr<Solver>`.
`Solver` exposes two operations:

- initialize a backend for a simulation box;
- advance a mutable `ParticleData` instance by one step.

Two concrete implementations use this boundary:

```text
Solver
├── CpuScalarSolver   (OpenMP + Uniform Grid)
└── CudaSolver        (CUDA + CudaUniformGrid)
```

`FluidSimulator` constructs the backend selected by `SimulationConfig`. GUI
and benchmark modes use this same selection path; CUDA is the default and the
CLI can select the CPU scalar backend. The CUDA solver mirrors the CPU solver's
three SPH stages, using one CUDA thread per active particle and a configurable
block size that defaults to 256 threads. `CudaUniformGrid` rebuilds cell counts,
exclusive offsets, and grouped particle indices on the device; density and
acceleration then traverse adjacent grid cells. Device allocations are created
once during initialization and reused for subsequent steps.

The normal and headless CUDA paths upload positions and velocities before each
step and copy them back afterward. With interop configured, the initial state
is uploaded once and the simulation SoA remains on the device across frames.

## Configuration and benchmark flow

`ApplicationConfig` groups `WindowConfig`, `CameraConfig`, and
`SimulationConfig`, plus `GraphicsRuntimeConfig`. Simulation settings in turn
contain `SimulationBoxConfig`, `ParticleSpawnConfig`, `SphParameters`, and
`CudaConfig`. The graphics settings contain the frames-in-flight count and the
interop flag. These structs provide the defaults used by GUI mode without
introducing an external configuration system.

`BenchmarkConfig` contains the same `SimulationConfig`, so the headless
benchmark creates the same `ParticleSystem` and `FluidSimulator` backend path.
It does not construct `Application` or initialize GLFW or Vulkan. Warmup steps
are excluded from the measured end-to-end simulation time; CUDA measurements
include the non-interop host/device copies.

The graphical interop benchmark instead creates two complete, independent
`Application` instances with identical simulation settings. It measures the
normal CUDA rendering path first and the interop path second.

The simulation details are covered in [Simulation](simulation.md).

## Vulkan/CUDA interoperability

In normal GUI mode, the interop path is enabled only when the CUDA backend and
`--interop` are both selected. The graphical interop benchmark enables each
mode directly for its corresponding run. Resource ownership remains within the
existing application, simulation, and graphics objects:

```text
Application
├── GraphicsRuntime
│   └── ParticleBuffer
│       ├── VulkanBuffer[]                     normal path
│       └── VulkanExportableBuffer[]
│           + ExportableSemaphore[]            interop path
│
└── FluidSimulator
    └── CudaSolver
        ├── CUDA SoA simulation buffers
        └── CudaInteropData[]
            ├── imported ParticleVertex*
            └── imported external semaphore
```

The simulation remains SoA: CUDA does not simulate directly in the Vulkan
vertex buffer. After a simulation step, `writeParticleVerticesKernel` packs the
three resident position arrays into the imported `ParticleVertex` buffer. This
POD contains three consecutive `float` fields and is the same buffer bound by
Vulkan for the draw.

There is one exportable render buffer and one external timeline semaphore per
frame-in-flight slot. These resources are indexed with
`Renderer::getCurrentFrame()`. The acquired swapchain image index independently
selects the command buffer, framebuffer, and presentation image; it is not used
as the CUDA interop slot.

Each slot starts at timeline value 0. For reuse number `N`, synchronization is:

```text
CUDA waits for 2N          Vulkan-done from the previous reuse
CUDA packs ParticleVertex
CUDA signals 2N + 1       CUDA-ready

Vulkan waits for 2N + 1 at the vertex-input stage
Vulkan draws the shared buffer
Vulkan signals 2N + 2     Vulkan-done for the next reuse
```

The swapchain's binary image-available and render-finished semaphores remain in
the same queue submission. Frame preparation acquires a valid image before
CUDA signals the shared slot, avoiding a stranded timeline value when the
swapchain is out of date.

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
- `ParticleBuffer` owns either the per-frame mapped `VulkanBuffer` objects or
  the per-frame `VulkanExportableBuffer` and `ExportableSemaphore` objects.
- `CommandRecorder` records a draw for a selected swapchain image.
- `FrameRenderer` coordinates acquire, submit, and present.
- `GraphicsRuntimeLifecycle` creates and recreates dependent resources in the
  required order.

## Current architectural constraints

- Backend selection occurs at process startup; there is no live backend
  switching.
- `ParticleData` remains host-owned, while interop mode keeps the evolving CUDA
  simulation state resident in separate device allocations.
- Rendering needs a `ParticleVertex` AoS representation because simulation
  data is stored as SoA. The CPU and non-interop CUDA paths build it on the
  host; the interop path builds it with a CUDA kernel.
- Host/device copies remain in every CUDA step when interop is disabled and in
  the headless simulation benchmark.
- Vulkan/CUDA physical-device UUID matching is not implemented; the code does
  not explicitly select the CUDA device corresponding to the Vulkan device.
- External-memory, external-semaphore, and timeline capability properties are
  not probed before initialization. Requested interop failures are reported
  instead of falling back automatically.
- External handles use Linux `OPAQUE_FD` file descriptors, so the interop path
  is currently Linux/NVIDIA-oriented.
- The CPU Uniform Grid rebuild is serial; only the CPU particle physics passes
  use OpenMP.
- `GridCell` remains in the source tree but is not used by the active flat-array
  Uniform Grid implementation.
- The project produces one executable and does not expose installable library
  targets.
