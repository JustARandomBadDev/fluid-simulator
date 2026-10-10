#ifndef FLUID_SIMULATOR_CUDA_SOLVER_HPP
#define FLUID_SIMULATOR_CUDA_SOLVER_HPP

#include <cstddef>
#include <cstdint>
#include <driver_types.h>
#include <vector>

#include "backends/cuda/cuda_sph_kernels.cuh"
#include "simulation/solver.hpp"

namespace fluid::graphics {
struct ParticleVertex;
}

namespace fluid::simulation {

struct CudaInteropData {
    cudaExternalMemory_t external_memory = nullptr;
    graphics::ParticleVertex* render_buffer = nullptr;
    cudaExternalSemaphore_t semaphore = nullptr;
    uint64_t syncValue = 0;
};

class CudaSolver : public Solver {
  public:
    CudaSolver(const SimulationConfig& config)
        : Solver(config), _box_config{config.box.position.x,
                              config.box.position.y,
                              config.box.position.z,
                              config.box.position.x + config.box.dimensions.x,
                              config.box.position.y + config.box.dimensions.y,
                              config.box.position.z + config.box.dimensions.z},
          _uniform_grid(config) {}
    CudaSolver(const CudaSolver&) = delete;
    CudaSolver& operator=(const CudaSolver&) = delete;

    ~CudaSolver();

    void configureInterop(std::size_t p_slot_count);
    void cleanupInterop();

    void init() override;
    void importRenderBuffer(
        int p_fd,
        std::size_t p_allocation_size,
        std::size_t p_buffer_size,
        std::size_t p_frame
    );
    void importCudaSemaphore(int p_fd, std::size_t p_frame);
    void step(ParticleData& p_particles, float p_dt) override;
    void writeRenderBuffer(
        std::size_t p_interop_slot,
        uint64_t& p_cuda_ready_value,
        uint64_t& p_vulkan_done_value
    );

    bool hasInterop() const {
        return !_interop_data.empty();
    }

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

    std::vector<CudaInteropData> _interop_data;
    bool _device_state_initialized = false;
    std::size_t _particle_count = 0;
};

} // namespace fluid::simulation

#endif
