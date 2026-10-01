#include "simulation/fluid_simulator.hpp"

#include <memory>
#include <stdexcept>

#include "backends/cpu/cpu_scalar_solver.hpp"
#include "backends/cuda/cuda_solver.hpp"

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

} // namespace fluid::simulation
