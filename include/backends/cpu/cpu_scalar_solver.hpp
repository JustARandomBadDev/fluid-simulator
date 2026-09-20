#ifndef FLUID_SIMULATOR_CPU_SCALAR_SOLVER_HPP
#define FLUID_SIMULATOR_CPU_SCALAR_SOLVER_HPP

#include <vector>

#include <glm/glm.hpp>

#include "simulation/solver.hpp"
#include "simulation/uniform_grid.hpp"

namespace fluid::simulation {

class CpuScalarSolver : public Solver {
public:
    CpuScalarSolver() = default;

    void init(glm::vec3 p_box_dim) override;

    void step(
        ParticleData& p_particles,
        float p_dt
    ) override;

private:
    glm::vec3 _box_dim;

    UniformGrid _grid;

    std::vector<glm::vec3> _accelerations;

    std::vector<float> _inverse_densities;
    std::vector<float> _pressure_terms;
};

}

#endif