#ifndef FLUID_SIMULATION_PARTICLE_DATA_HPP
#define FLUID_SIMULATION_PARTICLE_DATA_HPP

#include <cstddef>
#include <vector>

namespace fluid::simulation {

struct ParticleData {
    std::vector<float> position_x;
    std::vector<float> position_y;
    std::vector<float> position_z;

    std::vector<float> velocity_x;
    std::vector<float> velocity_y;
    std::vector<float> velocity_z;

    std::vector<float> densities;
    std::vector<float> pressures;

    std::size_t count{};
};

} // namespace fluid::simulation

#endif