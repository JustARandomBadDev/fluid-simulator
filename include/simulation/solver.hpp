#ifndef FLUID_SIMULATOR_SOLVER_HPP
#define FLUID_SIMULATOR_SOLVER_HPP

#include "simulation/particles_data.hpp"

namespace fluid::simulation {

class Solver {
public:
    virtual void init() = 0;
    virtual void step(
        ParticleData& p_particles,
        float p_dt
    ) = 0; 
};

}

#endif