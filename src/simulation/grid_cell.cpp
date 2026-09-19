#include "simulation/grid_cell.hpp"

namespace fluid::simulation {

void GridCell::init() {

}

void GridCell::clear() {
    _particles_index.clear();
}

void GridCell::add(std::size_t p_index) {
    _particles_index.push_back(p_index);
}

}
