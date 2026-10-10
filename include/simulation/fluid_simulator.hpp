#ifndef FLUID_SIMULATOR_FLUID_SIMULATOR_HPP
#define FLUID_SIMULATOR_FLUID_SIMULATOR_HPP

#include <cstddef>
#include <cstdint>
#include <memory>

#include "backends/cuda/cuda_solver.hpp"
#include "config.hpp"
#include "simulation/particle_system.hpp"
#include "simulation/solver.hpp"

namespace fluid::simulation {

struct CudaInteropSyncValues {
    uint64_t cudaReadyValue = 0;
    uint64_t vulkanDoneValue = 0;
};

class FluidSimulator {
  public:
    explicit FluidSimulator(const SimulationConfig& config);
    FluidSimulator(std::unique_ptr<Solver> p_solver)
        : _solver(std::move(p_solver)) {};

    void init(const SimulationConfig& config);
    const ParticleData& update(float dt);

    void configureCudaInterop(std::size_t p_slot_count);
    void importCudaInteropBuffer(
        int p_fd,
        std::size_t p_allocation_size,
        std::size_t p_buffer_size,
        std::size_t p_frame
    );
    void importCudaInteropSemaphore(int p_fd, std::size_t p_frame);
    CudaInteropSyncValues writeCudaRenderBuffer(std::size_t p_interop_slot);
    void cleanupCudaInterop();

  private:
    std::unique_ptr<Solver> _solver;
    ParticleSystem _particle_system;

    CudaSolver* getCudaSolver();
};

} // namespace fluid::simulation

#endif
