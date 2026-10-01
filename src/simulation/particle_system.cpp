#include "simulation/particle_system.hpp"

#include <cstdlib>
#include <stdexcept>

namespace fluid::simulation {

void ParticleSystem::init(const SimulationConfig& config) {
    clear();
    _capacity = config.particleCapacity;
    _initial_velocity = config.spawn.initialVelocity;

    _particles.position_x.resize(_capacity);
    _particles.position_y.resize(_capacity);
    _particles.position_z.resize(_capacity);

    _particles.velocity_x.resize(_capacity);
    _particles.velocity_y.resize(_capacity);
    _particles.velocity_z.resize(_capacity);

    const glm::vec3 randomDivisors{1000.f / config.spawn.dimensions.x,
        1000.f / config.spawn.dimensions.y,
        1000.f / config.spawn.dimensions.z};

    for (std::size_t i = 0; i < config.spawn.count; i++) {
        const glm::vec3 randomPosition{float(std::rand() % 1000) /
                                           randomDivisors.x,
            float(std::rand() % 1000) / randomDivisors.y,
            float(std::rand() % 1000) / randomDivisors.z};
        if (!addParticle(config.spawn.position + randomPosition)) {
            throw std::runtime_error(
                "ParticleSystem::initializeBox() -> capacity exceeded"
            );
        }
    }
}

bool ParticleSystem::addParticle(const glm::vec3 p_position) {
    std::size_t& count = _particles.count;

    if (count >= _capacity) return false;

    _particles.position_x[count] = p_position.x;
    _particles.position_y[count] = p_position.y;
    _particles.position_z[count] = p_position.z;

    _particles.velocity_x[count] = _initial_velocity.x;
    _particles.velocity_y[count] = _initial_velocity.y;
    _particles.velocity_z[count] = _initial_velocity.z;

    count++;

    return true;
}

bool ParticleSystem::removeParticle(std::size_t p_index) {
    std::size_t& count = _particles.count;

    if (p_index >= count) return false;

    const std::size_t last = count - 1;

    _particles.position_x[p_index] = _particles.position_x[last];
    _particles.position_y[p_index] = _particles.position_y[last];
    _particles.position_z[p_index] = _particles.position_z[last];

    _particles.velocity_x[p_index] = _particles.velocity_x[last];
    _particles.velocity_y[p_index] = _particles.velocity_y[last];
    _particles.velocity_z[p_index] = _particles.velocity_z[last];

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
