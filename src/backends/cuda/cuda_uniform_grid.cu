#include "backends/cuda/cuda_uniform_grid.hpp"

#include <cstddef>
#include <cstdint>

#include <cub/cub.cuh>
#include <cuda_runtime.h>

#include "backends/cuda/cuda_uniform_grid.cuh"

namespace fluid::simulation {

__global__ void updateParticlesCellKernel(
    float* p_particle_pos_x,
    float* p_particle_pos_y,
    float* p_particle_pos_z,
    std::size_t p_particles_count,
    float3 p_box_pos,
    uint3 p_dimensions,
    float p_inv_cell_size,
    uint32_t* p_particle_cells,
    int3* p_particle_cell_positions
) {
    const std::size_t index = blockIdx.x * blockDim.x + threadIdx.x;

    if (index >= p_particles_count) {
        return;
    }

    const float3 position = make_float3(
        p_particle_pos_x[index],
        p_particle_pos_y[index],
        p_particle_pos_z[index]
    );

    const int3 cell_position =
        positionToCell(position, p_box_pos, p_inv_cell_size);

    if (!isCellInsideGrid(cell_position, p_dimensions)) {
        return;
    }

    const uint32_t new_cell = getCellIndex(
        make_uint3(
            static_cast<unsigned int>(cell_position.x),
            static_cast<unsigned int>(cell_position.y),
            static_cast<unsigned int>(cell_position.z)
        ),
        p_dimensions
    );

    p_particle_cells[index] = new_cell;
    p_particle_cell_positions[index] = cell_position;
}

__global__ void updateCellCountsKernel(
    uint32_t* p_cell_counts,
    uint32_t* p_particle_cells,
    std::size_t p_particles_count
) {
    const std::size_t index = blockIdx.x * blockDim.x + threadIdx.x;

    if (index >= p_particles_count) {
        return;
    }

    const uint32_t cell = p_particle_cells[index];

    atomicAdd(&p_cell_counts[cell], 1u);
}

__global__ void updateParticleIndicesKernel(
    uint32_t* p_cell_offsets,
    uint32_t* p_particle_cells,
    uint32_t* p_particle_indices,
    uint32_t* p_current_cell_counts,
    std::size_t p_particles_count
) {
    const std::size_t index = blockIdx.x * blockDim.x + threadIdx.x;

    if (index >= p_particles_count) {
        return;
    }

    const uint32_t cell = p_particle_cells[index];

    const uint32_t local_index = atomicAdd(&p_current_cell_counts[cell], 1u);

    const uint32_t destination = p_cell_offsets[cell] + local_index;

    p_particle_indices[destination] = index;
}

void CudaUniformGrid::init(float p_cell_size) {
    _inv_cell_size = 1.0f / p_cell_size;

    const uint3 dimensions = getDimensions();

    _nb_cells = dimensions.x * dimensions.y * dimensions.z;

    const std::size_t cell_uint_size = sizeof(uint32_t) * _nb_cells;
    const std::size_t particle_uint_size =
        sizeof(uint32_t) * _config.particleCapacity;
    const std::size_t particle_ivec_size =
        sizeof(int3) * _config.particleCapacity;

    cudaMalloc(&_cell_counts, cell_uint_size);
    cudaMalloc(&_cell_offsets, cell_uint_size);
    cudaMalloc(&_current_cell_counts, cell_uint_size);

    cudaMalloc(&_particle_indices, particle_uint_size);
    cudaMalloc(&_particle_cells, particle_uint_size);
    cudaMalloc(&_particle_cell_positions, particle_ivec_size);

    cub::DeviceScan::ExclusiveSum(
        nullptr,
        _scan_temp_storage_bytes,
        _cell_counts,
        _cell_offsets,
        _nb_cells
    );

    cudaMalloc(&_scan_temp_storage, _scan_temp_storage_bytes);
}

void CudaUniformGrid::rebuild(
    float* p_particle_pos_x,
    float* p_particle_pos_y,
    float* p_particle_pos_z,
    std::size_t p_particles_count
) {
    const std::size_t threads = _config.cuda.blockSize;
    const std::size_t blocks = (p_particles_count + threads - 1) / threads;
    const uint3 dimensions = getDimensions();

    updateParticlesCellKernel<<<blocks, threads>>>(
        p_particle_pos_x,
        p_particle_pos_y,
        p_particle_pos_z,
        p_particles_count,
        make_float3(
            _config.box.position.x,
            _config.box.position.y,
            _config.box.position.z
        ),
        dimensions,
        _inv_cell_size,
        _particle_cells,
        _particle_cell_positions
    );

    cudaMemset(_cell_counts, 0, _nb_cells * sizeof(uint32_t));

    updateCellCountsKernel<<<blocks, threads>>>(
        _cell_counts,
        _particle_cells,
        p_particles_count
    );

    cub::DeviceScan::ExclusiveSum(
        _scan_temp_storage,
        _scan_temp_storage_bytes,
        _cell_counts,
        _cell_offsets,
        _nb_cells
    );

    cudaMemset(_current_cell_counts, 0, _nb_cells * sizeof(uint32_t));

    updateParticleIndicesKernel<<<blocks, threads>>>(
        _cell_offsets,
        _particle_cells,
        _particle_indices,
        _current_cell_counts,
        p_particles_count
    );

    _particle_count = p_particles_count;
    _initialized = true;
}

uint3 CudaUniformGrid::getDimensions() const {
    return make_uint3(
        static_cast<unsigned int>(_config.box.dimensions.x * _inv_cell_size) +
            1u,
        static_cast<unsigned int>(_config.box.dimensions.y * _inv_cell_size) +
            1u,
        static_cast<unsigned int>(_config.box.dimensions.z * _inv_cell_size) +
            1u
    );
}

CudaUniformGrid::~CudaUniformGrid() {
    cudaFree(_cell_counts);
    cudaFree(_cell_offsets);
    cudaFree(_particle_indices);
    cudaFree(_particle_cells);
    cudaFree(_particle_cell_positions);
    cudaFree(_current_cell_counts);
    cudaFree(_scan_temp_storage);
}

} // namespace fluid::simulation
