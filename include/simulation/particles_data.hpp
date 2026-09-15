#ifndef FLUID_SIMULATION_PARTICLE_DATA_HPP
#define FLUID_SIMULATION_PARTICLE_DATA_HPP

#include <cstddef>
#include <glm/glm.hpp>
#include <vector>

namespace fluid::simulation {

struct ParticleData {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> velocities;
    std::vector<float> densities;
    std::vector<float> pressures;
    std::size_t count;
};

} // namespace fluid::simulation

#endif
