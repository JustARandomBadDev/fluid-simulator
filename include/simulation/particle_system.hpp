#ifndef FLUID_SIMULATION_PARTICLE_SYSTEM_HPP
#define FLUID_SIMULATION_PARTICLE_SYSTEM_HPP

#include <array>
#include <cstddef>
#include <span>

#include "simulation/particle.hpp"

namespace fluid::simulation {

class ParticleSystem {
public:
    static constexpr std::size_t MAX_PARTICLES = 100'000;

    void init();

    bool addParticle(const Particle& p_particle);
    bool removeParticle(std::size_t p_index);

    void clear() noexcept;

    [[nodiscard]]
    std::span<const Particle> particles() const noexcept;

    [[nodiscard]]
    std::span<Particle> particles() noexcept;

    [[nodiscard]]
    std::size_t count() const noexcept;

private:
    std::array<Particle, MAX_PARTICLES> _particles{};
    std::size_t _count = 0;
};

} // namespace fluid::simulation

#endif
