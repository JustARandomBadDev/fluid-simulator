#include "backends/cpu/cpu_scalar_solver.hpp"
#include "simulation/particle.hpp"
#include <glm/ext/vector_float3.hpp>

namespace fluid::simulation {

void CpuScalarSolver::init() {

}

void CpuScalarSolver::step(
    std::span<Particle> particles,
    float dt
) {
    _time += dt;

    const float offset = std::sin(_time) * 0.1f;

    for (Particle& particle : particles) {
        particle.position.y += offset * dt;
    }
}

}