#ifndef FLUID_SIMULATOR_SOLVER_HPP
#define FLUID_SIMULATOR_SOLVER_HPP

#include "config.hpp"
#include "simulation/particles_data.hpp"
#include "simulation/sph_constants.hpp"
#include "simulation/sph_parameters.hpp"
#include <glm/ext/vector_float3.hpp>

namespace fluid::simulation {

class Solver {
  public:
    explicit Solver(const SimulationConfig &config)
        : _config(config), _params(config.sph),
          _constants(makeSphConstants(_params)) {}

    virtual ~Solver() = default;

    virtual void init() = 0;
    virtual void step(ParticleData &p_particles, float p_dt) = 0;

  protected:
    SimulationConfig _config;
    SphParameters _params;
    SphConstants _constants;
};

} // namespace fluid::simulation

#endif
