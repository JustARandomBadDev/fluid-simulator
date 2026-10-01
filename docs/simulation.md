# Simulation

[Back to the README](../README.md) · [Architecture](architecture.md)

The project implements two SPH backends behind the same `Solver` interface. The
scalar CPU solver uses a flat Uniform Grid and OpenMP work sharing. The initial
CUDA solver mirrors the same simulation stages with a deliberately naive GPU
implementation. The application currently selects the CUDA backend directly.

## Initial state and capacity

`ParticleSystem` reserves all arrays for 100,000 particles and initializes
20,000 active particles in a small region near the bottom of a 2 × 20 × 2
axis-aligned box. Initial velocities, densities, and pressures are zero.

The active count is tracked separately from capacity. Adding a particle writes
into the next preallocated slot. Removing a particle replaces it with the last
active particle, so removal does not preserve ordering.

## Particle data layout

`ParticleData` holds one `std::vector<float>` for each scalar component:

- position: `x`, `y`, and `z`;
- velocity: `x`, `y`, and `z`;
- density;
- pressure.

This SoA layout lets a simulation pass read only the components it needs and
keeps consecutive values for one field contiguous in memory. Renderer input is
different: the application gathers active positions into an array of
`ParticleVertex`, whose only field is a `glm::vec3`.

The CUDA solver allocates matching device arrays once for the maximum particle
capacity and reuses them across simulation steps.

## SPH parameters

The parameters are compile-time defaults in `SphParameters`; the application
does not currently expose a configuration file or runtime controls.

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
CUDA thread to an active particle and uses blocks of 256 threads.

### 1. Density and pressure

For each particle, both solvers reject neighbors outside the smoothing radius
and accumulate density with the Poly6 kernel factor. The CPU backend obtains
candidates from neighboring grid cells, while the CUDA kernel scans every
active particle. Pressure is clamped to a non-negative value based on the
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

## CPU Uniform Grid

The Uniform Grid cell size equals the SPH smoothing radius. Its dimensions are
computed from the simulation-box dimensions, with one additional cell on each
axis. A 3D cell coordinate is flattened as:

```text
x + y * width + z * width * height
```

The active implementation uses compact arrays:

- cell counts;
- prefix-sum cell offsets;
- particle indices grouped by cell;
- each particle's flattened cell index and 3D cell coordinate;
- temporary per-cell insertion counts.

During refresh, particle-to-cell assignments are calculated first. Counts,
offsets, and grouped particle indices are rebuilt only if the active particle
count or at least one particle's cell changes.

Because the cell width matches the smoothing radius, each SPH pass searches the
particle's cell and its immediate neighbors: a maximum of 27 grid cells instead
of scanning every active particle.

The CUDA backend does not use this grid yet.

## CUDA execution and data flow

`CudaSolver::init` allocates device buffers for positions, velocities,
densities, pressures, cached density terms, accelerations, box dimensions, and
SPH parameters/constants. These allocations persist until the solver is
destroyed.

Each simulation step currently performs:

1. six host-to-device `cudaMemcpy` operations for positions and velocities;
2. the density/pressure kernel;
3. the acceleration kernel;
4. the integration and box-collision kernel;
5. an explicit device synchronization;
6. eight device-to-host copies for positions, velocities, densities, and
   pressures.

The density and acceleration kernels compare each particle with every other
active particle. Their neighbor search is therefore O(n²). This implementation
is a correctness and profiling baseline rather than an optimized GPU solver.

The returned host positions are repacked into Vulkan vertices after the CUDA
step. CUDA and Vulkan use separate buffers; external-memory interoperability is
not implemented.

## Parallelization and performance scope

The CPU density/pressure, acceleration, and integration loops are parallelized
with OpenMP. Its grid refresh remains serial. The CUDA backend parallelizes one
particle per thread, but its all-pairs search and synchronous transfers remain
major limitations. Position repacking and upload to the mapped Vulkan buffer
also remain on the CPU.

The project does not currently include a benchmark target or published
performance measurements, so no CPU/CUDA throughput or frame-rate guarantees
are claimed. Planned CUDA work includes a GPU spatial grid, accelerated
neighbor lookup, improved memory access and shared-memory use, transfer
reduction, profiling, kernel optimization, and Vulkan/CUDA shared buffers.
