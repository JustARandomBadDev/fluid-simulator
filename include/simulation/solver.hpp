#ifndef FLUID_SIMULATOR_SOLVER_HPP
#define FLUID_SIMULATOR_SOLVER_HPP

#include <span>

#include "simulation/particle.hpp"

namespace fluid::simulation {

class Solver {
public:
    virtual void init() = 0;
    virtual void step(
        std::span<Particle> particles,
        float dt
    ) = 0; 
};

}

#endif