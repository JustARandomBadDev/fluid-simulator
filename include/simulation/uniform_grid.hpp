#ifndef FLUID_SIMULATOR_UNIFORM_GRID_HPP
#define FLUID_SIMULATOR_UNIFORM_GRID_HPP

#include <cstddef>
#include <vector>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int3.hpp>
#include <glm/ext/vector_uint3.hpp>

#include "simulation/grid_cell.hpp"
#include "simulation/particles_data.hpp"

namespace fluid::simulation {

class UniformGrid {
public:
    void init(glm::vec3 p_box_dim, float p_cell_size);

    GridCell& get(glm::uvec3 p_position);
    GridCell& get(
        std::size_t x,
        std::size_t y,
        std::size_t z
    );

    glm::ivec3 positionToCell(glm::vec3 p_position) const;
    bool contains(glm::ivec3 p_position) const;

    void build(const ParticleData& p_data);

    const std::vector<GridCell>& getCells() const;

private:
    glm::uvec3 _dimensions;
    float _cell_size{};
    std::vector<GridCell> _cells;
};

}

#endif
