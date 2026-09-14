#ifndef FLUID_SIMULATOR_SPU_SCALAR_SOLVER_HPP
#define FLUID_SIMULATOR_SPU_SCALAR_SOLVER_HPP

#include "simulation/solver.hpp"

namespace fluid::simulation {

class CpuScalarSolver : public Solver {
public:
    CpuScalarSolver() = default;
    void init() override;
    void step(
        std::span<Particle> particles,
        float dt
    ) override;

private:
    float _time = 0.f;
};

}

#endif