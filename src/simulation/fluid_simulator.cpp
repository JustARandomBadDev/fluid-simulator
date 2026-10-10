#include "simulation/fluid_simulator.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>

#include "backends/cpu/cpu_scalar_solver.hpp"

namespace fluid::simulation {

namespace {

std::unique_ptr<Solver> createSolver(const SimulationConfig& config) {
    switch (config.backend) {
    case SimulationBackend::CpuScalar:
        return std::make_unique<CpuScalarSolver>(config);
    case SimulationBackend::Cuda:
        return std::make_unique<CudaSolver>(config);
    }

    throw std::runtime_error("unsupported simulation backend");
}

} // namespace

FluidSimulator::FluidSimulator(const SimulationConfig& config)
    : _solver(createSolver(config)) {}

void FluidSimulator::init(const SimulationConfig& config) {
    const glm::vec3& boxDimensions = config.box.dimensions;
    if (boxDimensions.x < 0.f || boxDimensions.y < 0.f || boxDimensions.z < 0.f)
        throw std::runtime_error("Box dimension cannot be < 0");

    _particle_system.init(config);
    _solver->init();
}

const ParticleData& FluidSimulator::update(float p_dt) {
    if (!_solver) throw std::runtime_error("Empty solver !");

    _solver->step(_particle_system.particles(), p_dt);

    return _particle_system.particles();
}

void FluidSimulator::configureCudaInterop(std::size_t p_slot_count) {
    getCudaSolver()->configureInterop(p_slot_count);
}

void FluidSimulator::importCudaInteropBuffer(
    int p_fd,
    std::size_t p_allocation_size,
    std::size_t p_buffer_size,
    std::size_t p_frame
) {
    getCudaSolver()
        ->importRenderBuffer(p_fd, p_allocation_size, p_buffer_size, p_frame);
}

void FluidSimulator::importCudaInteropSemaphore(int p_fd, std::size_t p_frame) {
    getCudaSolver()->importCudaSemaphore(p_fd, p_frame);
}

CudaInteropSyncValues
FluidSimulator::writeCudaRenderBuffer(std::size_t p_interop_slot) {
    CudaInteropSyncValues values;
    getCudaSolver()->writeRenderBuffer(
        p_interop_slot,
        values.cudaReadyValue,
        values.vulkanDoneValue
    );
    return values;
}

CudaSolver* FluidSimulator::getCudaSolver() {
    auto* cudaSolver = dynamic_cast<CudaSolver*>(_solver.get());
    if (cudaSolver == nullptr) {
        throw std::runtime_error(
            "FluidSimulator::writeCudaRenderBuffer() -> CUDA backend is not "
            "active"
        );
    }

    return cudaSolver;
}

void FluidSimulator::cleanupCudaInterop() {
    auto* cudaSolver = dynamic_cast<CudaSolver*>(_solver.get());
    if (cudaSolver != nullptr) cudaSolver->cleanupInterop();
}

} // namespace fluid::simulation
