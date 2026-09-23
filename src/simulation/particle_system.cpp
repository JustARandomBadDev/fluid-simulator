#include "simulation/particle_system.hpp"

#include <cstdlib>
#include <stdexcept>

namespace fluid::simulation {

void ParticleSystem::init() {
    clear();

    _particles.position_x.resize(MAX_PARTICLES);
    _particles.position_y.resize(MAX_PARTICLES);
    _particles.position_z.resize(MAX_PARTICLES);

    _particles.velocity_x.resize(MAX_PARTICLES);
    _particles.velocity_y.resize(MAX_PARTICLES);
    _particles.velocity_z.resize(MAX_PARTICLES);

    _particles.densities.resize(MAX_PARTICLES);
    _particles.pressures.resize(MAX_PARTICLES);

    for (int i = 0; i < 2000; i++) {
        if (!addParticle(
                {float(std::rand() % 1000) / 2000.f + 0.25f,
                    float(std::rand() % 1000) / 1000.f + 0.50f,
                    float(std::rand() % 1000) / 2000.f + 0.25f}
            )) {
            throw std::runtime_error(
                "ParticleSystem::initializeBox() -> capacity exceeded"
            );
        }
    }
}

bool ParticleSystem::addParticle(const glm::vec3 p_position) {
    std::size_t &count = _particles.count;

    if (count >= MAX_PARTICLES) return false;

    _particles.position_x[count] = p_position.x;
    _particles.position_y[count] = p_position.y;
    _particles.position_z[count] = p_position.z;

    _particles.velocity_x[count] = 0.0f;
    _particles.velocity_y[count] = 0.0f;
    _particles.velocity_z[count] = 0.0f;

    _particles.densities[count] = 0.0f;
    _particles.pressures[count] = 0.0f;

    count++;

    return true;
}

bool ParticleSystem::removeParticle(std::size_t p_index) {
    std::size_t &count = _particles.count;

    if (p_index >= count) return false;

    const std::size_t last = count - 1;

    _particles.position_x[p_index] = _particles.position_x[last];
    _particles.position_y[p_index] = _particles.position_y[last];
    _particles.position_z[p_index] = _particles.position_z[last];

    _particles.velocity_x[p_index] = _particles.velocity_x[last];
    _particles.velocity_y[p_index] = _particles.velocity_y[last];
    _particles.velocity_z[p_index] = _particles.velocity_z[last];

    _particles.densities[p_index] = _particles.densities[last];
    _particles.pressures[p_index] = _particles.pressures[last];

    count--;

    return true;
}

void ParticleSystem::clear() noexcept {
    _particles.count = 0;
}

const ParticleData &ParticleSystem::particles() const noexcept {
    return _particles;
}

ParticleData &ParticleSystem::particles() noexcept {
    return _particles;
}

std::size_t ParticleSystem::count() const noexcept {
    return _particles.count;
}

} // namespace fluid::simulation