#ifndef FLUID_BACKENDS_CUDA_CUDA_SPH_KERNELS_CUH
#define FLUID_BACKENDS_CUDA_CUDA_SPH_KERNELS_CUH

#include <cstddef>

#include <vector_types.h>

#include "backends/cuda/cuda_uniform_grid.hpp"
#include "simulation/sph_constants.hpp"
#include "simulation/sph_parameters.hpp"

namespace fluid::simulation {

struct CudaParticleBuffers {
    float* position_x;
    float* position_y;
    float* position_z;

    float* velocity_x;
    float* velocity_y;
    float* velocity_z;

    std::size_t count;
};

struct CudaSphBuffers {
    float* inverse_densities;
    float* pressure_terms;

    float* acceleration_x;
    float* acceleration_y;
    float* acceleration_z;
};

struct CudaSphContext {
    SphParameters params;
    SphConstants constants;
    float3 gravity;
};

struct BoxConfig {
    const float x_min;
    const float y_min;
    const float z_min;

    const float x_max;
    const float y_max;
    const float z_max;
};

#ifdef __CUDACC__

__global__ void pressureAndDensityKernel(
    CudaParticleBuffers p_particles,
    CudaSphBuffers p_sph,
    CudaGridView p_grid,
    CudaSphContext p_context
);

__global__ void accelerationKernel(
    CudaParticleBuffers p_particles,
    CudaSphBuffers p_sph,
    CudaGridView p_grid,
    CudaSphContext p_context
);

__global__ void integrateKernel(
    CudaParticleBuffers p_particles,
    CudaSphBuffers p_sph,
    BoxConfig p_box_config,
    float p_dt
);

#endif

} // namespace fluid::simulation

#endif
