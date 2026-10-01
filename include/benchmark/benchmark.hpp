#ifndef FLUID_SIMULATOR_BENCHMARK_HPP
#define FLUID_SIMULATOR_BENCHMARK_HPP

#include <cstddef>

#include "config.hpp"

namespace fluid::benchmark {

struct BenchmarkConfig {
    SimulationConfig simulation;

    std::size_t warmupSteps = 100;
    std::size_t measuredSteps = 1000;

    float deltaTime = 0.001f;
};

int run(const BenchmarkConfig& config = {});

} // namespace fluid::benchmark

#endif
