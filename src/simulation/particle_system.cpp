#include "simulation/particle_system.hpp"

#include <cstdlib>
#include <stdexcept>

namespace fluid::simulation {

void ParticleSystem::init() {
    clear();

    for (int i = 0; i < 2000; i++) {
        const Particle particle{
            .position = {
                float(std::rand() % 1000) / 2000.f - 0.25f,
                float(std::rand() % 1000) / 1000.f + 0.50f,
                float(std::rand() % 1000) / 2000.f - 0.25f}
        };

        if (!addParticle(particle)) {
            throw std::runtime_error("ParticleSystem::initializeBox() -> capacity exceeded");
        }
    }
}

bool ParticleSystem::addParticle(const Particle& p_particle) {
    if (_count >= MAX_PARTICLES) {
        return false;
    }

    _particles[_count++] = p_particle;
    return true;
}

bool ParticleSystem::removeParticle(std::size_t p_index) {
    if (p_index >= _count) {
        return false;
    }

    _particles[p_index] = _particles[_count - 1];
    --_count;
    return true;
}

void ParticleSystem::clear() noexcept {
    _count = 0;
}

std::span<const Particle> ParticleSystem::particles() const noexcept {
    return {_particles.data(), _count};
}

std::span<Particle> ParticleSystem::particles() noexcept {
    return {_particles.data(), _count};
}

std::size_t ParticleSystem::count() const noexcept {
    return _count;
}

} // namespace fluid::simulation
