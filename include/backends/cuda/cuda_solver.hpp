#ifndef FLUID_SIMULATOR_CUDA_SOLVER_HPP
#define FLUID_SIMULATOR_CUDA_SOLVER_HPP

#include "backends/cuda/cuda_sph_kernels.cuh"
#include "simulation/solver.hpp"

namespace fluid::simulation {

class CudaSolver : public Solver {
  public:
    CudaSolver(const SimulationConfig& config)
        : Solver(config), _uniform_grid(config),
          _box_config{config.box.position.x,
              config.box.position.y,
              config.box.position.z,
              config.box.position.x + config.box.dimensions.x,
              config.box.position.y + config.box.dimensions.y,
              config.box.position.z + config.box.dimensions.z} {}
    CudaSolver(const CudaSolver&) = delete;
    CudaSolver& operator=(const CudaSolver&) = delete;

    ~CudaSolver();

    void init() override;
    void step(ParticleData& p_particles, float p_dt) override;

  private:
    float* _position_x{};
    float* _position_y{};
    float* _position_z{};

    float* _velocity_x{};
    float* _velocity_y{};
    float* _velocity_z{};

    float* _inverse_densities{};
    float* _pressure_terms{};

    float* _accelerations_x{};
    float* _accelerations_y{};
    float* _accelerations_z{};

    BoxConfig _box_config;
    CudaUniformGrid _uniform_grid;
};

} // namespace fluid::simulation

#endif
