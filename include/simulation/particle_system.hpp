#ifndef FLUID_SIMULATION_PARTICLE_SYSTEM_HPP
#define FLUID_SIMULATION_PARTICLE_SYSTEM_HPP

#include <cstddef>
#include <glm/ext/vector_float3.hpp>

#include "config.hpp"
#include "simulation/particles_data.hpp"

namespace fluid::simulation {

class ParticleSystem {
  public:
    void init(const SimulationConfig &config);

    bool addParticle(const glm::vec3 p_position);
    bool removeParticle(std::size_t p_index);

    void clear() noexcept;

    const ParticleData &particles() const noexcept;
    ParticleData &particles() noexcept;
    std::size_t count() const noexcept;

  private:
    ParticleData _particles;
    std::size_t _capacity{};
    glm::vec3 _initial_velocity{};
};

} // namespace fluid::simulation

#endif
