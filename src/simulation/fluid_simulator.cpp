#include "simulation/fluid_simulator.hpp"

#include <stdexcept>

namespace fluid::simulation {

void FluidSimulator::init(glm::vec3 p_box_dim) {
    if (p_box_dim.x < 0.f ||
        p_box_dim.y < 0.f ||
        p_box_dim.z < 0.f
    ) throw std::runtime_error("Box dimension cannot be < 0");

    _particle_system.init();
    _solver->init(p_box_dim);
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