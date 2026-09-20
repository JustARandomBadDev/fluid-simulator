#include "simulation/uniform_grid.hpp"
#include <cstddef>
#include <cstdint>

namespace fluid::simulation {

void UniformGrid::init(glm::vec3 p_box_dim, float p_cell_size, std::size_t p_max_particles) {
    _cell_size = p_cell_size;
    _dimensions = glm::uvec3(p_box_dim / p_cell_size) + glm::uvec3(1);
    _nb_cells = _dimensions.x * _dimensions.y * _dimensions.z;

    _cell_counts.resize(_nb_cells, 0);
    _cell_offsets.resize(_nb_cells, 0);
    _particle_indices.resize(p_max_particles, 0);
    _particle_cells.resize(p_max_particles);
    _current_cell_counts.resize(_nb_cells, 0);
}

uint32_t UniformGrid::get(glm::uvec3 p_position) {
    const uint32_t index =
        p_position.x +
        p_position.y * _dimensions.x +
        p_position.z * _dimensions.x * _dimensions.y;

    return index;
}

uint32_t UniformGrid::get(
    std::size_t p_x,
    std::size_t p_y,
    std::size_t p_z
) {
    return get(glm::uvec3(p_x, p_y, p_z));
}

glm::ivec3 UniformGrid::positionToCell(glm::vec3 p_position) const {
    return glm::ivec3(p_position / _cell_size);
}

bool UniformGrid::contains(glm::ivec3 p_position) const {
    return
        p_position.x >= 0 &&
        p_position.y >= 0 &&
        p_position.z >= 0 &&
        p_position.x < static_cast<int>(_dimensions.x) &&
        p_position.y < static_cast<int>(_dimensions.y) &&
        p_position.z < static_cast<int>(_dimensions.z);
}

bool UniformGrid::rebuild(const ParticleData& p_data) {
    bool changed = false;

    for (uint32_t i = 0; i < p_data.count; i++) {
        const uint32_t new_cell =
            get(positionToCell(p_data.positions[i]));

        if (new_cell != _particle_cells[i]) {
            _particle_cells[i] = new_cell;
            changed = true;
        }
    }

    if (!changed)
        return false;

    // rebuild compact representation
    std::fill(
        _cell_counts.begin(),
        _cell_counts.end(),
        0
    );

    for (uint32_t i = 0; i < p_data.count; i++) {
        _cell_counts[_particle_cells[i]]++;
    }

    uint32_t count = 0;

    for (uint32_t i = 0; i < _nb_cells; i++) {
        _cell_offsets[i] = count;
        count += _cell_counts[i];
    }

    std::fill(
        _current_cell_counts.begin(),
        _current_cell_counts.end(),
        0
    );

    for (uint32_t i = 0; i < p_data.count; i++) {
        const uint32_t cell = _particle_cells[i];

        _particle_indices[
            _cell_offsets[cell] +
            _current_cell_counts[cell]
        ] = i;

        _current_cell_counts[cell]++;
    }

    return true;
}

}
