#include "simulation/uniform_grid.hpp"

namespace fluid::simulation {

void UniformGrid::init(glm::vec3 p_box_dim, float p_cell_size) {
    _cell_size = p_cell_size;
    _dimensions = glm::uvec3(p_box_dim / p_cell_size) + glm::uvec3(1);
    _cells.resize(_dimensions.x * _dimensions.y * _dimensions.z);
}

GridCell& UniformGrid::get(glm::uvec3 p_position) {
    const std::size_t index =
        p_position.x +
        p_position.y * _dimensions.x +
        p_position.z * _dimensions.x * _dimensions.y;

    return _cells[index];
}

GridCell& UniformGrid::get(
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

const std::vector<GridCell>& UniformGrid::getCells() const {
    return _cells;
}

void UniformGrid::build(const ParticleData& p_data) {
    for (auto& cell : _cells)
        cell.clear();

    for (std::size_t index = 0; index < p_data.count; ++index) {
        const glm::ivec3 cell_pos = positionToCell(p_data.positions[index]);

        if (!contains(cell_pos))
            continue;

        get(glm::uvec3(cell_pos)).add(index);
    }
}

}
