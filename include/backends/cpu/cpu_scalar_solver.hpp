#ifndef FLUID_SIMULATOR_CPU_SCALAR_SOLVER_HPP
#define FLUID_SIMULATOR_CPU_SCALAR_SOLVER_HPP

#include <numbers>

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
    static constexpr float BOX_SIZE = 1.f;
    static constexpr float PARTICLE_MASS = 0.12f;
    static constexpr float SMOOTHING_RADIUS = 0.10f;
    static constexpr float REST_DENSITY = 1000.0f;

    static constexpr float STIFFNESS = 40.0f;
    static constexpr float VISCOSITY = 0.03f;
    static constexpr float DAMPING = 0.1f;

    static constexpr float H6 =
        SMOOTHING_RADIUS *
        SMOOTHING_RADIUS *
        SMOOTHING_RADIUS *
        SMOOTHING_RADIUS *
        SMOOTHING_RADIUS *
        SMOOTHING_RADIUS;

    static constexpr float H9 =
        H6 *
        SMOOTHING_RADIUS *
        SMOOTHING_RADIUS *
        SMOOTHING_RADIUS;

    static constexpr float POLY_SIX =
        315.0f /
        (64.0f * std::numbers::pi_v<float> * H9);

    static constexpr float SPIKY =
        45.0f /
        (std::numbers::pi_v<float> * H6);
};

}

#endif