#include "backends/cuda/cuda_sph_kernels.cuh"

#include <cstddef>
#include <cstdint>

#include <cuda_runtime.h>

#include "backends/cuda/cuda_uniform_grid.cuh"

namespace fluid::simulation {
namespace {

__device__ __forceinline__ void
applyAxisCollision(float& position, float& velocity, float min, float max) {
    if (position < min) {
        position = min;
        velocity = 0.0f;
    } else if (position > max) {
        position = max;
        velocity = 0.0f;
    }
}

__device__ void applyBoxCollision(
    float& p_position_x,
    float& p_position_y,
    float& p_position_z,

    float& p_velocity_x,
    float& p_velocity_y,
    float& p_velocity_z,

    const BoxConfig p_box_config
) {
    applyAxisCollision(
        p_position_x,
        p_velocity_x,
        p_box_config.x_min,
        p_box_config.x_max
    );
    applyAxisCollision(
        p_position_y,
        p_velocity_y,
        p_box_config.y_min,
        p_box_config.y_max
    );
    applyAxisCollision(
        p_position_z,
        p_velocity_z,
        p_box_config.z_min,
        p_box_config.z_max
    );
}

} // namespace

__global__ void pressureAndDensityKernel(
    CudaParticleBuffers p_particles,
    CudaSphBuffers p_sph,
    CudaGridView p_grid,
    CudaSphContext p_context
) {
    const std::size_t index = blockIdx.x * blockDim.x + threadIdx.x;

    if (index >= p_particles.count) return;

    const float pix = p_particles.position_x[index];
    const float piy = p_particles.position_y[index];
    const float piz = p_particles.position_z[index];

    const int3 cell_pos = p_grid.particle_cell_positions[index];

    float density = 0.0f;

    for (int z = -1; z <= 1; z++) {
        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                const int3 neighbor_pos =
                    make_int3(cell_pos.x + x, cell_pos.y + y, cell_pos.z + z);

                if (!isCellInsideGrid(neighbor_pos, p_grid.dimensions))
                    continue;

                const uint32_t cell_index = getCellIndex(
                    make_uint3(
                        static_cast<unsigned int>(neighbor_pos.x),
                        static_cast<unsigned int>(neighbor_pos.y),
                        static_cast<unsigned int>(neighbor_pos.z)
                    ),
                    p_grid.dimensions
                );

                const uint32_t offset = p_grid.cell_offsets[cell_index];
                const uint32_t count = p_grid.cell_counts[cell_index];
                const uint32_t end = offset + count;

                for (uint32_t k = offset; k < end; k++) {
                    const uint32_t j = p_grid.particle_indices[k];

                    const float rx = pix - p_particles.position_x[j];
                    const float ry = piy - p_particles.position_y[j];
                    const float rz = piz - p_particles.position_z[j];

                    const float r2 = rx * rx + ry * ry + rz * rz;

                    if (r2 >= p_context.constants.h2) continue;

                    const float q = p_context.constants.h2 - r2;

                    density += p_context.constants.density_factor * q * q * q;
                }
            }
        }
    }

    const float pressure = fmaxf(
        p_context.params.stiffness * (density - p_context.params.restDensity),
        0.0f
    );
    const float safe_density = fmaxf(density, 1e-6f);
    const float inverse_density = 1.0f / safe_density;

    p_sph.inverse_densities[index] = inverse_density;
    p_sph.pressure_terms[index] = pressure * inverse_density * inverse_density;
}

__global__ void accelerationKernel(
    CudaParticleBuffers p_particles,
    CudaSphBuffers p_sph,
    CudaGridView p_grid,
    CudaSphContext p_context
) {
    const std::size_t index = blockIdx.x * blockDim.x + threadIdx.x;

    if (index >= p_particles.count) return;

    const float pix = p_particles.position_x[index];
    const float piy = p_particles.position_y[index];
    const float piz = p_particles.position_z[index];

    const float vix = p_particles.velocity_x[index];
    const float viy = p_particles.velocity_y[index];
    const float viz = p_particles.velocity_z[index];

    const float pressure_i = p_sph.pressure_terms[index];

    float pressure_x = 0.0f;
    float pressure_y = 0.0f;
    float pressure_z = 0.0f;

    float viscosity_x = 0.0f;
    float viscosity_y = 0.0f;
    float viscosity_z = 0.0f;

    const int3 cell_pos = p_grid.particle_cell_positions[index];

    for (int z = -1; z <= 1; z++) {
        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                const int3 neighbor_pos =
                    make_int3(cell_pos.x + x, cell_pos.y + y, cell_pos.z + z);

                if (!isCellInsideGrid(neighbor_pos, p_grid.dimensions))
                    continue;

                const uint32_t cell_index = getCellIndex(
                    make_uint3(
                        static_cast<unsigned int>(neighbor_pos.x),
                        static_cast<unsigned int>(neighbor_pos.y),
                        static_cast<unsigned int>(neighbor_pos.z)
                    ),
                    p_grid.dimensions
                );

                const uint32_t offset = p_grid.cell_offsets[cell_index];
                const uint32_t count = p_grid.cell_counts[cell_index];
                const uint32_t end = offset + count;

                for (uint32_t k = offset; k < end; k++) {
                    const uint32_t j = p_grid.particle_indices[k];

                    if (index == j) continue;

                    const float rx = pix - p_particles.position_x[j];
                    const float ry = piy - p_particles.position_y[j];
                    const float rz = piz - p_particles.position_z[j];

                    const float r2 = rx * rx + ry * ry + rz * rz;

                    if (r2 <= 0.0f || r2 >= p_context.constants.h2) continue;

                    const float r = sqrtf(r2);
                    const float inverse_r = 1.0f / r;

                    const float distance_to_edge =
                        p_context.params.smoothingRadius - r;

                    const float distance_to_edge2 =
                        distance_to_edge * distance_to_edge;

                    const float pressure_scalar =
                        p_context.constants.pressure_factor *
                        (pressure_i + p_sph.pressure_terms[j]) *
                        distance_to_edge2 * inverse_r;

                    pressure_x += rx * pressure_scalar;
                    pressure_y += ry * pressure_scalar;
                    pressure_z += rz * pressure_scalar;

                    const float viscosity_scalar =
                        p_context.constants.viscosity_factor *
                        p_sph.inverse_densities[j] * distance_to_edge;

                    viscosity_x +=
                        (p_particles.velocity_x[j] - vix) * viscosity_scalar;
                    viscosity_y +=
                        (p_particles.velocity_y[j] - viy) * viscosity_scalar;
                    viscosity_z +=
                        (p_particles.velocity_z[j] - viz) * viscosity_scalar;
                }
            }
        }
    }

    p_sph.acceleration_x[index] =
        pressure_x + viscosity_x + p_context.gravity.x;
    p_sph.acceleration_y[index] =
        pressure_y + viscosity_y + p_context.gravity.y;
    p_sph.acceleration_z[index] =
        pressure_z + viscosity_z + p_context.gravity.z;
}

__global__ void integrateKernel(
    CudaParticleBuffers p_particles,
    CudaSphBuffers p_sph,
    BoxConfig p_box_config,
    float p_dt
) {
    const std::size_t index = blockIdx.x * blockDim.x + threadIdx.x;

    if (index >= p_particles.count) return;

    float velocity_x =
        p_particles.velocity_x[index] + p_sph.acceleration_x[index] * p_dt;
    float velocity_y =
        p_particles.velocity_y[index] + p_sph.acceleration_y[index] * p_dt;
    float velocity_z =
        p_particles.velocity_z[index] + p_sph.acceleration_z[index] * p_dt;

    float position_x = p_particles.position_x[index] + velocity_x * p_dt;
    float position_y = p_particles.position_y[index] + velocity_y * p_dt;
    float position_z = p_particles.position_z[index] + velocity_z * p_dt;

    applyBoxCollision(
        position_x,
        position_y,
        position_z,

        velocity_x,
        velocity_y,
        velocity_z,

        p_box_config
    );

    p_particles.position_x[index] = position_x;
    p_particles.position_y[index] = position_y;
    p_particles.position_z[index] = position_z;

    p_particles.velocity_x[index] = velocity_x;
    p_particles.velocity_y[index] = velocity_y;
    p_particles.velocity_z[index] = velocity_z;
}

} // namespace fluid::simulation
