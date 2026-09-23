#ifndef FLUID_SIMULATOR_UNIFORM_GRID_HPP
#define FLUID_SIMULATOR_UNIFORM_GRID_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int3.hpp>
#include <glm/ext/vector_uint3.hpp>

#include "simulation/particles_data.hpp"

namespace fluid::simulation {

class UniformGrid {
  public:
    void
    init(glm::vec3 p_box_dim, float p_cell_size, std::size_t p_max_particles);

    uint32_t get(glm::uvec3 p_position) const;

    uint32_t get(std::size_t p_x, std::size_t p_y, std::size_t p_z) const;

    uint32_t getCount(uint32_t p_index) const {
        return _cell_counts[p_index];
    }

    uint32_t getOffset(uint32_t p_index) const {
        return _cell_offsets[p_index];
    }

    uint32_t getParticleIndex(uint32_t p_index) const {
        return _particle_indices[p_index];
    }

    uint32_t getParticleCell(std::size_t p_index) const {
        return _particle_cells[p_index];
    }

    const glm::ivec3 &getParticleCellPosition(std::size_t p_index) const {
        return _particle_cell_positions[p_index];
    }

    glm::ivec3 positionToCell(glm::vec3 p_position) const;

    bool contains(glm::ivec3 p_position) const;

    bool rebuild(const ParticleData &p_data);

  private:
    glm::uvec3 _dimensions;

    float _cell_size{};
    float _inv_cell_size{};

    std::size_t _nb_cells{};

    std::vector<uint32_t> _cell_counts;
    std::vector<uint32_t> _cell_offsets;
    std::vector<uint32_t> _particle_indices;

    std::vector<uint32_t> _particle_cells;
    std::vector<glm::ivec3> _particle_cell_positions;

    std::vector<uint32_t> _current_cell_counts;

    std::size_t _particle_count{};
    bool _initialized{};
};

} // namespace fluid::simulation

#endif