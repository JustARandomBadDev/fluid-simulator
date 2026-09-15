#ifndef FLUID_SIMULATION_PARTICLE_HPP
#define FLUID_SIMULATION_PARTICLE_HPP

#include <glm/glm.hpp>

namespace fluid::simulation {

struct Particle {
    glm::vec3 position {};
    glm::vec3 velocity {0.f};
    float density = 0.f;
    float pressure = 0.f;
};

} // namespace fluid::simulation

#endif
