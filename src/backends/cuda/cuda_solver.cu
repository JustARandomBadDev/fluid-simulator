#include "backends/cuda/cuda_solver.hpp"
#include "simulation/sph_constants.hpp"
#include "simulation/sph_parameters.hpp"

#include <cstddef>
#include <cstdio>
#include <cuda_runtime.h>

namespace fluid::simulation {

__device__ void applyBoxCollision(
    float* p_position_x,
    float* p_position_y,
    float* p_position_z,

    float* p_velocity_x,
    float* p_velocity_y,
    float* p_velocity_z,

    const BoxDim* p_box_dim
) {
    if (p_box_dim->y > 0.0f) {
        if (*p_position_y < p_box_dim->position_y) {
            *p_position_y = p_box_dim->position_y;
            *p_velocity_y = 0.0f;
        } else if (*p_position_y > p_box_dim->position_y + p_box_dim->y) {
            *p_position_y = p_box_dim->position_y + p_box_dim->y;
            *p_velocity_y = 0.0f;
        }
    }

    if (p_box_dim->x > 0.0f) {
        if (*p_position_x < p_box_dim->position_x) {
            *p_position_x = p_box_dim->position_x;
            *p_velocity_x = 0.0f;
        } else if (*p_position_x > p_box_dim->position_x + p_box_dim->x) {
            *p_position_x = p_box_dim->position_x + p_box_dim->x;
            *p_velocity_x = 0.0f;
        }
    }

    if (p_box_dim->z > 0.0f) {
        if (*p_position_z < p_box_dim->position_z) {
            *p_position_z = p_box_dim->position_z;
            *p_velocity_z = 0.0f;
        } else if (*p_position_z > p_box_dim->position_z + p_box_dim->z) {
            *p_position_z = p_box_dim->position_z + p_box_dim->z;
            *p_velocity_z = 0.0f;
        }
    }
}

__global__ void pressureAndDensityKernel(
    const float* p_position_x,
    const float* p_position_y,
    const float* p_position_z,

    float* p_densities,
    float* p_pressures,

    float* p_inverse_densities,
    float* p_pressure_terms,

    const std::size_t p_count,

    const SphParameters* p_params,
    const SphConstants* p_constants
) {
    const std::size_t index = blockIdx.x * blockDim.x + threadIdx.x;

    if (index >= p_count) return;

    const float pix = p_position_x[index];
    const float piy = p_position_y[index];
    const float piz = p_position_z[index];

    float density = 0.0f;

    for (std::size_t j = 0; j < p_count; j++) {
        const float rx = pix - p_position_x[j];
        const float ry = piy - p_position_y[j];
        const float rz = piz - p_position_z[j];

        const float r2 = rx * rx + ry * ry + rz * rz;

        if (r2 >= p_constants->h2) continue;

        const float q = p_constants->h2 - r2;

        density += p_constants->density_factor * q * q * q;
    }

    const float pressure =
        fmaxf(p_params->stiffness * (density - p_params->restDensity), 0.0f);

    const float safe_density = fmaxf(density, 1e-6f);

    const float inverse_density = 1.0f / safe_density;

    p_densities[index] = density;
    p_pressures[index] = pressure;

    p_inverse_densities[index] = inverse_density;

    p_pressure_terms[index] = pressure * inverse_density * inverse_density;
}

__global__ void accelerationKernel(
    const float* p_position_x,
    const float* p_position_y,
    const float* p_position_z,

    const float* p_velocity_x,
    const float* p_velocity_y,
    const float* p_velocity_z,

    const float* p_inverse_densities,
    const float* p_pressure_terms,

    float* p_accelerations_x,
    float* p_accelerations_y,
    float* p_accelerations_z,

    const std::size_t p_count,

    const SphParameters* p_params,
    const SphConstants* p_constants,
    const float p_gravity_x,
    const float p_gravity_y,
    const float p_gravity_z
) {
    const std::size_t index = blockIdx.x * blockDim.x + threadIdx.x;

    if (index >= p_count) return;

    const float pix = p_position_x[index];
    const float piy = p_position_y[index];
    const float piz = p_position_z[index];

    const float vix = p_velocity_x[index];
    const float viy = p_velocity_y[index];
    const float viz = p_velocity_z[index];

    const float pressure_i = p_pressure_terms[index];

    float pressure_x = 0.0f;
    float pressure_y = 0.0f;
    float pressure_z = 0.0f;

    float viscosity_x = 0.0f;
    float viscosity_y = 0.0f;
    float viscosity_z = 0.0f;

    for (std::size_t j = 0; j < p_count; j++) {
        if (index == j) continue;

        const float rx = pix - p_position_x[j];
        const float ry = piy - p_position_y[j];
        const float rz = piz - p_position_z[j];

        const float r2 = rx * rx + ry * ry + rz * rz;

        if (r2 <= 0.0f || r2 >= p_constants->h2) continue;

        const float r = sqrtf(r2);
        const float inverse_r = 1.0f / r;

        const float distance_to_edge = p_params->smoothingRadius - r;

        const float distance_to_edge2 = distance_to_edge * distance_to_edge;

        const float pressure_scalar = p_constants->pressure_factor *
                                      (pressure_i + p_pressure_terms[j]) *
                                      distance_to_edge2 * inverse_r;

        pressure_x += rx * pressure_scalar;
        pressure_y += ry * pressure_scalar;
        pressure_z += rz * pressure_scalar;

        const float viscosity_scalar = p_constants->viscosity_factor *
                                       p_inverse_densities[j] *
                                       distance_to_edge;

        viscosity_x += (p_velocity_x[j] - vix) * viscosity_scalar;

        viscosity_y += (p_velocity_y[j] - viy) * viscosity_scalar;

        viscosity_z += (p_velocity_z[j] - viz) * viscosity_scalar;
    }

    p_accelerations_x[index] = pressure_x + viscosity_x + p_gravity_x;

    p_accelerations_y[index] = pressure_y + viscosity_y + p_gravity_y;

    p_accelerations_z[index] = pressure_z + viscosity_z + p_gravity_z;
}

__global__ void integrateKernel(
    float* p_position_x,
    float* p_position_y,
    float* p_position_z,

    float* p_velocity_x,
    float* p_velocity_y,
    float* p_velocity_z,

    const float* p_accelerations_x,
    const float* p_accelerations_y,
    const float* p_accelerations_z,

    const BoxDim* p_box_dim,

    const std::size_t p_count,
    const float p_dt
) {
    const std::size_t index = blockIdx.x * blockDim.x + threadIdx.x;

    if (index >= p_count) return;

    float* position_x = p_position_x + index;
    float* position_y = p_position_y + index;
    float* position_z = p_position_z + index;

    float* velocity_x = p_velocity_x + index;
    float* velocity_y = p_velocity_y + index;
    float* velocity_z = p_velocity_z + index;

    *velocity_x += p_accelerations_x[index] * p_dt;

    *velocity_y += p_accelerations_y[index] * p_dt;

    *velocity_z += p_accelerations_z[index] * p_dt;

    *position_x += *velocity_x * p_dt;
    *position_y += *velocity_y * p_dt;
    *position_z += *velocity_z * p_dt;

    applyBoxCollision(
        position_x,
        position_y,
        position_z,

        velocity_x,
        velocity_y,
        velocity_z,

        p_box_dim
    );
}

void CudaSolver::init() {
    BoxDim box_dim{_config.box.position.x,
        _config.box.position.y,
        _config.box.position.z,
        _config.box.dimensions.x,
        _config.box.dimensions.y,
        _config.box.dimensions.z};

    const std::size_t buffer_size = _config.particleCapacity * sizeof(float);

    cudaMalloc(&_position_x, buffer_size);
    cudaMalloc(&_position_y, buffer_size);
    cudaMalloc(&_position_z, buffer_size);

    cudaMalloc(&_velocity_x, buffer_size);
    cudaMalloc(&_velocity_y, buffer_size);
    cudaMalloc(&_velocity_z, buffer_size);

    cudaMalloc(&_densities, buffer_size);
    cudaMalloc(&_pressures, buffer_size);

    cudaMalloc(&_inverse_densities, buffer_size);
    cudaMalloc(&_pressure_terms, buffer_size);

    cudaMalloc(&_accelerations_x, buffer_size);
    cudaMalloc(&_accelerations_y, buffer_size);
    cudaMalloc(&_accelerations_z, buffer_size);

    cudaMalloc(&_box_dim, sizeof(BoxDim));

    cudaMemcpy(_box_dim, &box_dim, sizeof(BoxDim), cudaMemcpyHostToDevice);

    cudaMalloc(&_cuda_params, sizeof(SphParameters));

    cudaMalloc(&_cuda_constants, sizeof(SphConstants));

    cudaMemcpy(
        _cuda_params,
        &_params,
        sizeof(SphParameters),
        cudaMemcpyHostToDevice
    );

    cudaMemcpy(
        _cuda_constants,
        &_constants,
        sizeof(SphConstants),
        cudaMemcpyHostToDevice
    );
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

    const std::size_t threads = _config.cuda.blockSize;

    const std::size_t blocks = (p_particles.count + threads - 1) / threads;

    pressureAndDensityKernel<<<blocks, threads>>>(
        _position_x,
        _position_y,
        _position_z,

        _densities,
        _pressures,

        _inverse_densities,
        _pressure_terms,

        p_particles.count,

        _cuda_params,
        _cuda_constants
    );

    if (cudaGetLastError() != cudaSuccess) {
        std::printf("CUDA error: pressureAndDensityKernel\n");
    }

    accelerationKernel<<<blocks, threads>>>(
        _position_x,
        _position_y,
        _position_z,

        _velocity_x,
        _velocity_y,
        _velocity_z,

        _inverse_densities,
        _pressure_terms,

        _accelerations_x,
        _accelerations_y,
        _accelerations_z,

        p_particles.count,

        _cuda_params,
        _cuda_constants,
        _config.gravity.x,
        _config.gravity.y,
        _config.gravity.z
    );

    if (cudaGetLastError() != cudaSuccess) {
        std::printf("CUDA error: accelerationKernel\n");
    }

    integrateKernel<<<blocks, threads>>>(
        _position_x,
        _position_y,
        _position_z,

        _velocity_x,
        _velocity_y,
        _velocity_z,

        _accelerations_x,
        _accelerations_y,
        _accelerations_z,

        _box_dim,

        p_particles.count,
        p_dt
    );

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

    cudaFree(_densities);
    cudaFree(_pressures);

    cudaFree(_inverse_densities);
    cudaFree(_pressure_terms);

    cudaFree(_accelerations_x);
    cudaFree(_accelerations_y);
    cudaFree(_accelerations_z);

    cudaFree(_box_dim);

    cudaFree(_cuda_constants);
    cudaFree(_cuda_params);
}

} // namespace fluid::simulation
