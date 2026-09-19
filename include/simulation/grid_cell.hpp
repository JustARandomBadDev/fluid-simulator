#ifndef FLUID_SIMULATOR_GRID_CELL_HPP
#define FLUID_SIMULATOR_GRID_CELL_HPP

#include <cstddef>
#include <vector>

namespace fluid::simulation {

class GridCell {
public:
    void init();
    void clear();

    void add(std::size_t p_index);

    const std::vector<std::size_t>& get() const { return _particles_index; }

private:
    std::vector<std::size_t> _particles_index;
};

}

#endif