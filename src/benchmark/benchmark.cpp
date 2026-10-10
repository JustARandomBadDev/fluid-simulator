#include "benchmark/benchmark.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>

#include "simulation/fluid_simulator.hpp"

namespace fluid::benchmark {
namespace {

const char* backendName(SimulationBackend backend) {
    switch (backend) {
    case SimulationBackend::CpuScalar:
        return "CPU";
    case SimulationBackend::Cuda:
        return "CUDA";
    }

    return "Unknown";
}

} // namespace

int run(const BenchmarkConfig& config) {
    if (config.measuredSteps == 0) {
        throw std::runtime_error(
            "benchmark measured step count must be greater than 0"
        );
    }

    simulation::FluidSimulator simulator(config.simulation);
    simulator.init(config.simulation);

    for (std::size_t step = 0; step < config.warmupSteps; step++) {
        simulator.update(config.deltaTime);
    }

    const auto start = std::chrono::steady_clock::now();

    for (std::size_t step = 0; step < config.measuredSteps; step++) {
        simulator.update(config.deltaTime);
    }

    const auto end = std::chrono::steady_clock::now();
    const double totalSeconds =
        std::chrono::duration<double>(end - start).count();
    const double averageMilliseconds =
        totalSeconds * 1000.0 / static_cast<double>(config.measuredSteps);
    const double stepsPerSecond =
        static_cast<double>(config.measuredSteps) / totalSeconds;
    const double particlesPerSecond =
        static_cast<double>(config.simulation.spawn.count) * stepsPerSecond;

    std::cout << "Fluid Simulator Benchmark\n\n"
              << "Backend: " << backendName(config.simulation.backend) << '\n'
              << "Particles: " << config.simulation.spawn.count << '\n'
              << "Warmup steps: " << config.warmupSteps << '\n'
              << "Measured steps: " << config.measuredSteps << '\n'
              << "Delta time: " << config.deltaTime << " s\n\n"
              << std::fixed << std::setprecision(3)
              << "Total time: " << totalSeconds << " s\n"
              << "Average step: " << averageMilliseconds << " ms\n"
              << std::setprecision(2) << "Steps/s: " << stepsPerSecond << '\n'
              << "Particles/s: " << particlesPerSecond << "\n";

    return 0;
}

} // namespace fluid::benchmark
