#ifndef FLUID_SIMULATION_PARTICLE_SYSTEM_HPP
#define FLUID_SIMULATION_PARTICLE_SYSTEM_HPP

#include <cstddef>

#include "simulation/particles_data.hpp"

namespace fluid::simulation {

class ParticleSystem {
public:
    static constexpr std::size_t MAX_PARTICLES = 100'000;

    void init();

    bool addParticle(const glm::vec3 p_position);
    bool removeParticle(std::size_t p_index);

    void clear() noexcept;

    const ParticleData& particles() const noexcept;
    ParticleData& particles() noexcept;
    std::size_t count() const noexcept;

private:
    ParticleData _particles;
};

} // namespace fluid::simulation

#endif
