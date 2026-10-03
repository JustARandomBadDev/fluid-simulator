#include "backends/cuda/cuda_solver.hpp"

#include <cstddef>
#include <cstdio>
#include <cuda_runtime.h>

namespace fluid::simulation {

void CudaSolver::init() {
    _uniform_grid.init(_params.smoothingRadius);

    const std::size_t buffer_size = _config.particleCapacity * sizeof(float);

    cudaMalloc(&_position_x, buffer_size);
    cudaMalloc(&_position_y, buffer_size);
    cudaMalloc(&_position_z, buffer_size);

    cudaMalloc(&_velocity_x, buffer_size);
    cudaMalloc(&_velocity_y, buffer_size);
    cudaMalloc(&_velocity_z, buffer_size);

    cudaMalloc(&_inverse_densities, buffer_size);
    cudaMalloc(&_pressure_terms, buffer_size);

    cudaMalloc(&_accelerations_x, buffer_size);
    cudaMalloc(&_accelerations_y, buffer_size);
    cudaMalloc(&_accelerations_z, buffer_size);
}

void CudaSolver::step(ParticleData& p_particles, float p_dt) {
    if (p_particles.count == 0) return;

    p_dt = std::min(p_dt, _config.maximumTimeStep);

    const std::size_t size = p_particles.count * sizeof(float);

    cudaMemcpy(
        _position_x,
        p_particles.position_x.data(),
        size,
        cudaMemcpyHostToDevice
    );

    cudaMemcpy(
        _position_y,
        p_particles.position_y.data(),
        size,
        cudaMemcpyHostToDevice
    );

    cudaMemcpy(
        _position_z,
        p_particles.position_z.data(),
        size,
        cudaMemcpyHostToDevice
    );

    cudaMemcpy(
        _velocity_x,
        p_particles.velocity_x.data(),
        size,
        cudaMemcpyHostToDevice
    );

    cudaMemcpy(
        _velocity_y,
        p_particles.velocity_y.data(),
        size,
        cudaMemcpyHostToDevice
    );

    cudaMemcpy(
        _velocity_z,
        p_particles.velocity_z.data(),
        size,
        cudaMemcpyHostToDevice
    );

    _uniform_grid
        .rebuild(_position_x, _position_y, _position_z, p_particles.count);

    const std::size_t threads = _config.cuda.blockSize;

    const std::size_t blocks = (p_particles.count + threads - 1) / threads;

    const CudaParticleBuffers particles{.position_x = _position_x,
        .position_y = _position_y,
        .position_z = _position_z,
        .velocity_x = _velocity_x,
        .velocity_y = _velocity_y,
        .velocity_z = _velocity_z,
        .count = p_particles.count};

    const CudaSphBuffers sph{.inverse_densities = _inverse_densities,
        .pressure_terms = _pressure_terms,
        .acceleration_x = _accelerations_x,
        .acceleration_y = _accelerations_y,
        .acceleration_z = _accelerations_z};

    const CudaGridView grid = _uniform_grid.getView();

    const CudaSphContext context{.params = _params,
        .constants = _constants,
        .gravity = make_float3(
            _config.gravity.x,
            _config.gravity.y,
            _config.gravity.z
        )};

    pressureAndDensityKernel<<<blocks, threads>>>(
        particles,
        sph,
        grid,
        context
    );

    if (cudaGetLastError() != cudaSuccess) {
        std::printf("CUDA error: pressureAndDensityKernel\n");
    }

    accelerationKernel<<<blocks, threads>>>(particles, sph, grid, context);

    if (cudaGetLastError() != cudaSuccess) {
        std::printf("CUDA error: accelerationKernel\n");
    }

    integrateKernel<<<blocks, threads>>>(particles, sph, _box_config, p_dt);

    cudaError_t error = cudaDeviceSynchronize();

    if (error != cudaSuccess) {
        std::printf("CUDA runtime error: %s\n", cudaGetErrorString(error));
    }

    cudaMemcpy(
        p_particles.position_x.data(),
        _position_x,
        size,
        cudaMemcpyDeviceToHost
    );

    cudaMemcpy(
        p_particles.position_y.data(),
        _position_y,
        size,
        cudaMemcpyDeviceToHost
    );

    cudaMemcpy(
        p_particles.position_z.data(),
        _position_z,
        size,
        cudaMemcpyDeviceToHost
    );

    cudaMemcpy(
        p_particles.velocity_x.data(),
        _velocity_x,
        size,
        cudaMemcpyDeviceToHost
    );

    cudaMemcpy(
        p_particles.velocity_y.data(),
        _velocity_y,
        size,
        cudaMemcpyDeviceToHost
    );

    cudaMemcpy(
        p_particles.velocity_z.data(),
        _velocity_z,
        size,
        cudaMemcpyDeviceToHost
    );
}

CudaSolver::~CudaSolver() {
    cudaFree(_position_x);
    cudaFree(_position_y);
    cudaFree(_position_z);

    cudaFree(_velocity_x);
    cudaFree(_velocity_y);
    cudaFree(_velocity_z);

    cudaFree(_inverse_densities);
    cudaFree(_pressure_terms);

    cudaFree(_accelerations_x);
    cudaFree(_accelerations_y);
    cudaFree(_accelerations_z);
}

} // namespace fluid::simulation
