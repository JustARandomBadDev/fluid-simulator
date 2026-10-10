# Simulation

[Back to the README](../README.md) · [Architecture](architecture.md)

The project implements two SPH backends behind the same `Solver` interface. The
scalar CPU solver uses a flat Uniform Grid and OpenMP work sharing. The CUDA
solver mirrors the same simulation stages and uses a device-side Uniform Grid.
GUI and benchmark modes select either implementation through the shared
`SimulationConfig`; CUDA is the default backend.

## Initial state and capacity

`ParticleSystem` reserves all arrays for 100,000 particles and initializes
100,000 active particles in a 2 × 5.5 × 2 region starting at `(1, 0.5, 1)`
inside a 4 × 6 × 4 axis-aligned box. Initial velocities are zero. Capacity,
spawn settings, box bounds, and velocity are defaults in `SimulationConfig`.

The active count is tracked separately from capacity. Adding a particle writes
into the next preallocated slot. Removing a particle replaces it with the last
active particle, so removal does not preserve ordering.

## Particle data layout

`ParticleData` holds one `std::vector<float>` for each scalar component:

- position: `x`, `y`, and `z`;
- velocity: `x`, `y`, and `z`.

This SoA layout lets a simulation pass read only the components it needs and
keeps consecutive values for one field contiguous in memory. Renderer input is
different: `ParticleVertex` is an AoS POD containing three `float` fields. The
application fills it on the host in the normal rendering path, while CUDA fills
the imported Vulkan buffer directly when interop is enabled.

The CUDA solver allocates matching device arrays once for the maximum particle
capacity and reuses them across simulation steps.

## SPH parameters

`SimulationConfig` contains `SphParameters`, gravity, and the maximum time step.
Their default values are listed below; the application does not currently
expose a configuration file or runtime controls for them.

| Parameter          |       Current value |
| ------------------ | ------------------: |
| Particle mass      |              `0.12` |
| Smoothing radius   |               `0.1` |
| Rest density       |            `1000.0` |
| Pressure stiffness |              `40.0` |
| Viscosity          |              `0.03` |
| Gravity            | `(0.0, -9.81, 0.0)` |
| Maximum step       |           `0.001 s` |

`SphConstants` derives the squared, sixth, and ninth powers of the smoothing
radius along with the Poly6 and Spiky kernel factors when a solver is created.

## Simulation step

Both solvers cap the supplied frame delta at 1 ms and execute the same three
stages. `CpuScalarSolver` refreshes the Uniform Grid and enters one OpenMP
parallel region containing three `omp for schedule(static)` passes. The
implicit barrier at the end of each pass keeps their dependencies ordered.

`CudaSolver` launches a separate kernel for each stage. Each kernel assigns one
CUDA thread to an active particle. `CudaConfig` supplies the launch block size,
which defaults to 256 threads.

### 1. Density and pressure

For each particle, both solvers obtain candidates from neighboring grid cells,
reject neighbors outside the smoothing radius, and accumulate density with the
Poly6 kernel factor. Pressure is clamped to a non-negative value based on the
configured rest density and stiffness.

The pass also caches inverse density and the pressure term used by the force
pass. Density is clamped to a small positive value only for the reciprocal, to
avoid division by zero.

### 2. Acceleration

The second pass revisits the neighbors and accumulates pressure and viscosity
contributions. Self-interaction and zero-distance pairs are skipped for the
force calculation. Gravity is then added to the resulting acceleration.

### 3. Integration and collision

The final pass updates velocity and then position using the current
acceleration and capped time step. Positions are clamped independently against
the six box planes. When a particle reaches a plane, the velocity component
normal to that plane is set to zero; there is no restitution or friction model.

## Uniform Grids

The Uniform Grid cell size equals the SPH smoothing radius. Its dimensions are
computed from the simulation-box dimensions, with one additional cell on each
axis. A 3D cell coordinate is flattened as:

```text
x + y * width + z * width * height
```

The CPU implementation uses compact arrays:

- cell counts;
- prefix-sum cell offsets;
- particle indices grouped by cell;
- each particle's flattened cell index and 3D cell coordinate;
- temporary per-cell insertion counts.

During a CPU refresh, particle-to-cell assignments are calculated first.
Counts, offsets, and grouped particle indices are rebuilt only if the active
particle count or at least one particle's cell changes.

Because the cell width matches the smoothing radius, each SPH pass searches the
particle's cell and its immediate neighbors: a maximum of 27 grid cells instead
of scanning every active particle.

The CUDA backend uses the same cell size and 27-cell neighborhood. Its grid
rebuild runs on the device: kernels assign particles to cells, count them with
atomics, and scatter their indices after a CUB exclusive prefix scan.

## CUDA execution and data flow

`CudaSolver::init` allocates device buffers for positions, velocities, cached
density and pressure terms, accelerations, and the grid arrays. These
allocations persist until the solver is destroyed.

Every CUDA step rebuilds `CudaUniformGrid`, then launches the density/pressure,
acceleration, and integration/collision kernels. The first two kernels obtain
candidate particles from the adjacent grid cells.

Without interop, each step uploads the six position/velocity arrays and copies
them back afterward. The returned positions are packed into `ParticleVertex`
objects on the CPU and copied to a host-visible Vulkan buffer.

With interop, the initial SoA state is uploaded once and remains resident in
CUDA memory. After the SPH kernels, a separate CUDA kernel packs positions into
the imported Vulkan `ParticleVertex` buffer. The simulation layout itself
remains SoA; only its rendering representation uses AoS. External timeline
semaphores prevent CUDA from overwriting a frame slot while Vulkan reads it and
prevent Vulkan from drawing before CUDA has finished writing.

## Parallelization and performance scope

The CPU density/pressure, acceleration, and integration loops are parallelized
with OpenMP. Its grid refresh remains serial. The CUDA backend parallelizes one
particle per thread and rebuilds its grid on the GPU. Synchronous transfers and
CPU-side position packing remain in the non-interop path; interop avoids those
per-frame rendering transfers.

The executable includes a headless benchmark mode that uses the real
`FluidSimulator` update path. By default it runs 100 warmup steps followed by
1,000 measured steps at a 1 ms delta and reports total time, average step time,
steps per second, and particles per second. CUDA measurements include the
non-interop transfers. The separate `--benchmark-interop` mode compares the
two CUDA rendering paths using complete graphical frames. Historical results
and the benchmark methodology are recorded in
[Benchmark History](benchmarks.md); they are measurements rather than
throughput or frame-rate guarantees.
