# Simulation

[Back to the README](../README.md) · [Architecture](architecture.md)

The implemented simulation backend is a scalar CPU SPH solver. It uses a
Structure-of-Arrays particle layout, a flat Uniform Grid for neighborhood
lookup, and OpenMP work sharing for per-particle computations.

## Initial state and capacity

`ParticleSystem` reserves all arrays for 100,000 particles and initializes
2,000 active particles in a small region near the bottom of a 1 × 20 × 1
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

`CpuScalarSolver::step` caps the supplied frame delta at 1 ms, refreshes the
Uniform Grid, and enters one OpenMP parallel region. The region contains three
`omp for schedule(static)` passes. The implicit barrier at the end of each pass
keeps their dependencies ordered.

### 1. Density and pressure

For each particle, the solver visits candidates from neighboring grid cells,
rejects candidates outside the smoothing radius, and accumulates density with
the Poly6 kernel factor. Pressure is clamped to a non-negative value based on
the configured rest density and stiffness.

The pass also caches inverse density and the pressure term used by the force
pass. Density is clamped to a small positive value only for the reciprocal, to
avoid division by zero.

### 2. Acceleration

The second pass revisits the local neighbors and accumulates pressure and
viscosity contributions. Self-interaction and zero-distance pairs are skipped
for the force calculation. Gravity is then added to the resulting acceleration.

### 3. Integration and collision

The final pass updates velocity and then position using the current
acceleration and capped time step. Positions are clamped independently against
the six box planes. When a particle reaches a plane, the velocity component
normal to that plane is set to zero; there is no restitution or friction model.

## Uniform Grid

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

## Parallelization and performance scope

The density/pressure, acceleration, and integration loops are parallelized with
OpenMP. Grid refresh, position repacking for rendering, and upload to the mapped
Vulkan buffer remain serial. The project does not currently include a benchmark
target or published performance measurements, so no particle throughput or
frame-rate guarantees are claimed.

The solver is scalar C++; there are no explicit SIMD intrinsics and no CUDA
backend. CUDA simulation and Vulkan/CUDA interoperability are planned work, not
current capabilities.
