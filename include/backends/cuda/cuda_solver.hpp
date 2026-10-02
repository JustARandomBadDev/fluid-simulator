#ifndef FLUID_SIMULATOR_CUDA_SOLVER_HPP
#define FLUID_SIMULATOR_CUDA_SOLVER_HPP

#include "backends/cuda/cuda_uniform_grid.hpp"
#include "simulation/solver.hpp"
#include "simulation/sph_constants.hpp"
#include "simulation/sph_parameters.hpp"

namespace {

struct BoxDim {
    const float position_x;
    const float position_y;
    const float position_z;
    const float x;
    const float y;
    const float z;
};

} // namespace

namespace fluid::simulation {

class CudaSolver : public Solver {
  public:
    explicit CudaSolver(const SimulationConfig& config)
        : Solver(config), _uniform_grid(config) {}
    CudaSolver(const CudaSolver&) = delete;

    ~CudaSolver();

    void init() override;
    void step(ParticleData& p_particles, float p_dt) override;

  private:
    float* _position_x;
    float* _position_y;
    float* _position_z;

    float* _velocity_x;
    float* _velocity_y;
    float* _velocity_z;

    float* _densities;
    float* _pressures;

    float* _inverse_densities;
    float* _pressure_terms;

    float* _accelerations_x;
    float* _accelerations_y;
    float* _accelerations_z;

    BoxDim* _box_dim;

    SphParameters* _cuda_params;
    SphConstants* _cuda_constants;

    CudaUniformGrid _uniform_grid;
};

} // namespace fluid::simulation

#endif
