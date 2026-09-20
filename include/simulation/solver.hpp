#ifndef FLUID_SIMULATOR_SOLVER_HPP
#define FLUID_SIMULATOR_SOLVER_HPP

#include "simulation/particles_data.hpp"
#include "simulation/sph_constants.hpp"
#include "simulation/sph_parameters.hpp"
#include <glm/ext/vector_float3.hpp>

namespace fluid::simulation {

class Solver {
public:
    Solver()
    : _params(), _constants(makeSphConstants(_params)) {}
    
    virtual ~Solver() = default;

    virtual void init(glm::vec3 p_box_dim) = 0;
    virtual void step(
        ParticleData& p_particles,
        float p_dt
    ) = 0; 

protected:
    SphParameters _params;
    SphConstants _constants;
};

}

#endif