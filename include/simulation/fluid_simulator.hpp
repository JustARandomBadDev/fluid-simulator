#ifndef FLUID_SIMULATOR_FLUID_SIMULATOR_HPP
#define FLUID_SIMULATOR_FLUID_SIMULATOR_HPP

#include <memory>

#include "simulation/particle_system.hpp"
#include "simulation/solver.hpp"

namespace fluid::simulation {

class FluidSimulator {
  public:
    FluidSimulator(std::unique_ptr<Solver> p_solver)
        : _solver(std::move(p_solver)) {};

    void init(glm::vec3 p_box_dim);
    const ParticleData &update(float dt);

  private:
    std::unique_ptr<Solver> _solver;
    ParticleSystem _particle_system;
};

} // namespace fluid::simulation

#endif