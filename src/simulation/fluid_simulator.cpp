#include "simulation/fluid_simulator.hpp"

#include <stdexcept>

namespace fluid::simulation {

void FluidSimulator::init() {
    _particle_system.init();
    _solver->init();
}

const ParticleData& FluidSimulator::update(float p_dt) {
    if (! _solver) throw std::runtime_error("Empty solver !");
    
    _solver->step(
        _particle_system.particles(),
        p_dt
    );

    return _particle_system.particles();
}

}