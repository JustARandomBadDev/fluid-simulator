#include "simulation/particle_system.hpp"

#include <cmath>
#include <stdexcept>

namespace fluid::simulation {

void ParticleSystem::initializeBox(
    glm::vec3 p_center,
    glm::ivec3 p_dimensions,
    float p_spacing
) {
    if (p_dimensions.x <= 0 || p_dimensions.y <= 0 || p_dimensions.z <= 0) {
        throw std::runtime_error("ParticleSystem::initializeBox() -> dimensions must be strictly positive");
    }
    if (!std::isfinite(p_spacing) || p_spacing <= 0.0f) {
        throw std::runtime_error("ParticleSystem::initializeBox() -> spacing must be finite and strictly positive");
    }

    clear();

    const glm::vec3 gridCenter = {
        static_cast<float>(p_dimensions.x - 1) * 0.5f,
        static_cast<float>(p_dimensions.y - 1) * 0.5f,
        static_cast<float>(p_dimensions.z - 1) * 0.5f
    };

    for (int z = 0; z < p_dimensions.z; ++z) {
        for (int y = 0; y < p_dimensions.y; ++y) {
            for (int x = 0; x < p_dimensions.x; ++x) {
                const Particle particle{
                    .position = {
                        p_center.x + (static_cast<float>(x) - gridCenter.x) * p_spacing,
                        p_center.y + (static_cast<float>(y) - gridCenter.y) * p_spacing,
                        p_center.z + (static_cast<float>(z) - gridCenter.z) * p_spacing
                    },
                    .velocity = {}
                };

                if (!addParticle(particle)) {
                    throw std::runtime_error("ParticleSystem::initializeBox() -> capacity exceeded");
                }
            }
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

std::size_t ParticleSystem::count() const noexcept {
    return _count;
}

} // namespace fluid::simulation
