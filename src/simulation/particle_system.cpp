#include "simulation/particle_system.hpp"

#include <cstdlib>
#include <stdexcept>

namespace fluid::simulation {

void ParticleSystem::init() {
    clear();

    _particles.positions.resize(MAX_PARTICLES);
    _particles.velocities.resize(MAX_PARTICLES);
    _particles.densities.resize(MAX_PARTICLES);
    _particles.pressures.resize(MAX_PARTICLES);

    for (int i = 0; i < 2000; i++) {
        if (!addParticle({
            float(std::rand() % 1000) / 2000.f + 0.25f,
            float(std::rand() % 1000) / 1000.f + 0.50f,
            float(std::rand() % 1000) / 2000.f + 0.25f}
        )) {
            throw std::runtime_error("ParticleSystem::initializeBox() -> capacity exceeded");
        }
    }
}

bool ParticleSystem::addParticle(const glm::vec3 p_position) {
    std::size_t& count = _particles.count;

    if (count >= MAX_PARTICLES) return false;

    _particles.positions[count] = p_position;
    _particles.velocities[count] = {0.f, 0.f, 0.f};
    _particles.densities[count] = 0.f;
    _particles.pressures[count] = 0.f;

    count++;
    return true;
}

bool ParticleSystem::removeParticle(std::size_t p_index) {
    std::size_t& count = _particles.count;

    if (p_index >= count) return false;

    _particles.positions[p_index]  = _particles.positions[count - 1];
    _particles.velocities[p_index] = _particles.velocities[count - 1];
    _particles.densities[p_index]  = _particles.densities[count - 1];
    _particles.pressures[p_index]  = _particles.pressures[count - 1];

    count--;
    
    return true;
}

void ParticleSystem::clear() noexcept {
    _particles.count = 0;
}

const ParticleData& ParticleSystem::particles() const noexcept {
    return _particles;
}

ParticleData& ParticleSystem::particles() noexcept {
    return _particles;
}

std::size_t ParticleSystem::count() const noexcept {
    return _particles.count;
}

} // namespace fluid::simulation
