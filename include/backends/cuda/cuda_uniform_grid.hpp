#ifndef FLUID_BACKENDS_CUDA_CUDA_UNIFORM_GRID_HPP
#define FLUID_BACKENDS_CUDA_CUDA_UNIFORM_GRID_HPP

#include <cstddef>
#include <cstdint>

#include <vector_types.h>

#include "config.hpp"

namespace fluid::simulation {

struct CudaGridView {
    const uint32_t* cell_counts;
    const uint32_t* cell_offsets;
    const uint32_t* particle_indices;
    const int3* particle_cell_positions;

    uint3 dimensions;
};

class CudaUniformGrid {
  public:
    CudaUniformGrid(const SimulationConfig& p_config) : _config(p_config) {}
    CudaUniformGrid(const CudaUniformGrid&) = delete;
    CudaUniformGrid& operator=(const CudaUniformGrid&) = delete;

    ~CudaUniformGrid();

    void init(float p_cell_size);

    void rebuild(
        float* p_particle_pos_x,
        float* p_particle_pos_y,
        float* p_particle_pos_z,
        std::size_t p_particles_count
    );

    CudaGridView getView() const {
        return {_cell_counts,
            _cell_offsets,
            _particle_indices,
            _particle_cell_positions,
            getDimensions()};
    }

  private:
    SimulationConfig _config;

    float _inv_cell_size{};

    std::size_t _nb_cells{};

    uint32_t* _cell_counts{};
    uint32_t* _cell_offsets{};
    uint32_t* _particle_indices{};

    uint32_t* _particle_cells{};
    int3* _particle_cell_positions{};

    uint32_t* _current_cell_counts{};

    void* _scan_temp_storage{};
    std::size_t _scan_temp_storage_bytes{};

    std::size_t _particle_count{};
    bool _initialized{};

    uint3 getDimensions() const;
};

} // namespace fluid::simulation

#endif
