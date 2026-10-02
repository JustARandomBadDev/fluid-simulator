#ifndef FLUID_BACKENDS_CUDA_CUDA_UNIFORM_GRID_HPP
#define FLUID_BACKENDS_CUDA_CUDA_UNIFORM_GRID_HPP

#include <cstddef>
#include <cstdint>

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int3.hpp>
#include <glm/ext/vector_uint3.hpp>

#include "config.hpp"

namespace fluid::simulation {

class CudaUniformGrid {
  public:
    CudaUniformGrid(const SimulationConfig& p_config) : _config(p_config) {}

    ~CudaUniformGrid();

    void init(
        glm::vec3 p_position,
        glm::vec3 p_box_dim,
        float p_cell_size,
        std::size_t p_max_particles
    );

    void rebuild(
        float* p_particle_pos_x,
        float* p_particle_pos_y,
        float* p_particle_pos_z,
        std::size_t p_particles_count
    );

    uint32_t* getCounts() const {
        return _cell_counts;
    }

    uint32_t* getOffsets() const {
        return _cell_offsets;
    }

    uint32_t* getParticleIndices() const {
        return _particle_indices;
    }

    glm::ivec3* getParticleCellPositions() const {
        return _particle_cell_positions;
    }

    glm::uvec3 getDimensions() const {
        return _dimensions;
    }

  private:
    SimulationConfig _config;

    glm::vec3 _position;
    glm::uvec3 _dimensions;

    float _cell_size{};
    float _inv_cell_size{};

    std::size_t _nb_cells{};
    std::size_t _max_particles{};

    uint32_t* _cell_counts{};
    uint32_t* _cell_offsets{};
    uint32_t* _particle_indices{};

    uint32_t* _particle_cells{};
    glm::ivec3* _particle_cell_positions{};

    uint32_t* _current_cell_counts{};

    void* _scan_temp_storage{};
    std::size_t _scan_temp_storage_bytes{};

    std::size_t _particle_count{};
    bool _initialized{};
};

} // namespace fluid::simulation

#endif