#ifndef FLUID_SIMULATOR_CPU_SCALAR_SOLVER_HPP
#define FLUID_SIMULATOR_CPU_SCALAR_SOLVER_HPP

#include <vector>

#include <glm/glm.hpp>

#include "simulation/solver.hpp"
#include "simulation/uniform_grid.hpp"

namespace fluid::simulation {

class CpuScalarSolver : public Solver {
  public:
    explicit CpuScalarSolver(const SimulationConfig &config) : Solver(config) {}

    void init() override;
    void step(ParticleData &p_particles, float p_dt) override;

  private:
    void computeDensityAndPressure(ParticleData &p_particles);
    void computeAccelerations(ParticleData &p_particles);
    void integrate(ParticleData &p_particles, float p_dt);

    void applyBoxCollision(
        float &p_position_x,
        float &p_position_y,
        float &p_position_z,
        float &p_velocity_x,
        float &p_velocity_y,
        float &p_velocity_z
    ) const;

  private:
    glm::vec3 _box_position;
    glm::vec3 _box_dim;

    UniformGrid _grid;

    std::vector<glm::vec3> _accelerations;
    std::vector<float> _inverse_densities;
    std::vector<float> _pressure_terms;
};

} // namespace fluid::simulation

#endif
