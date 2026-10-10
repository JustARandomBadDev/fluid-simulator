#include "benchmark/interop_benchmark.hpp"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "app/application.hpp"
#include "config.hpp"

namespace fluid::benchmark {
namespace {

constexpr std::size_t kWarmupFrames = 100;
constexpr std::size_t kMeasuredFrames = 1000;
constexpr float kDeltaTime = 0.001f;
constexpr unsigned int kParticleSeed = 1;

struct FrameStatistics {
    double averageMilliseconds = 0.0;
    double medianMilliseconds = 0.0;
    double minimumMilliseconds = 0.0;
    double maximumMilliseconds = 0.0;
    double framesPerSecond = 0.0;
};

std::vector<double>
runMode(const ApplicationConfig& p_base_config, bool p_enable_interop) {
    ApplicationConfig config = p_base_config;
    config.graphics.enableCudaInterop = p_enable_interop;

    std::srand(kParticleSeed);

    try {
        Application application(std::move(config));
        return application
            .runFrameBenchmark(kWarmupFrames, kMeasuredFrames, kDeltaTime);
    } catch (const std::exception& exception) {
        throw std::runtime_error(
            std::string(
                p_enable_interop ? "interop run failed: "
                                 : "non-interop run failed: "
            ) +
            exception.what()
        );
    }
}

FrameStatistics calculateStatistics(const std::vector<double>& p_samples) {
    if (p_samples.empty()) {
        throw std::runtime_error("interop benchmark produced no frame samples");
    }

    std::vector<double> sortedSamples = p_samples;
    std::sort(sortedSamples.begin(), sortedSamples.end());

    FrameStatistics statistics;
    statistics.minimumMilliseconds = sortedSamples.front();
    statistics.maximumMilliseconds = sortedSamples.back();

    const std::size_t middle = sortedSamples.size() / 2;
    if (sortedSamples.size() % 2 == 0) {
        statistics.medianMilliseconds =
            (sortedSamples[middle - 1] + sortedSamples[middle]) / 2.0;
    } else {
        statistics.medianMilliseconds = sortedSamples[middle];
    }

    const double totalMilliseconds =
        std::accumulate(p_samples.begin(), p_samples.end(), 0.0);
    statistics.averageMilliseconds =
        totalMilliseconds / static_cast<double>(p_samples.size());
    statistics.framesPerSecond = 1000.0 / statistics.averageMilliseconds;
    return statistics;
}

void printStatistics(const char* p_label, const FrameStatistics& p_statistics) {
    std::cout << p_label << '\n'
              << std::fixed << std::setprecision(3)
              << "  Average: " << p_statistics.averageMilliseconds << " ms\n"
              << "  Median:  " << p_statistics.medianMilliseconds << " ms\n"
              << "  Min:     " << p_statistics.minimumMilliseconds << " ms\n"
              << "  Max:     " << p_statistics.maximumMilliseconds << " ms\n"
              << std::setprecision(2)
              << "  FPS:     " << p_statistics.framesPerSecond << "\n\n";
}

} // namespace

int runInterop() {
    ApplicationConfig baseConfig;
    baseConfig.simulation.backend = SimulationBackend::Cuda;

    std::cout << "CUDA/Vulkan interop benchmark\n"
              << "Particles: " << baseConfig.simulation.spawn.count << '\n'
              << "Warmup frames: " << kWarmupFrames << '\n'
              << "Measured frames: " << kMeasuredFrames << '\n'
              << "Delta time: " << kDeltaTime << " s\n\n";

    const std::vector<double> withoutInterop = runMode(baseConfig, false);
    const std::vector<double> withInterop = runMode(baseConfig, true);

    const FrameStatistics withoutStatistics =
        calculateStatistics(withoutInterop);
    const FrameStatistics withStatistics = calculateStatistics(withInterop);

    printStatistics("Without interop", withoutStatistics);
    printStatistics("With interop", withStatistics);

    const double averageDifference =
        (withStatistics.averageMilliseconds /
                withoutStatistics.averageMilliseconds -
            1.0) *
        100.0;
    const double throughputDifference =
        (withStatistics.framesPerSecond / withoutStatistics.framesPerSecond -
            1.0) *
        100.0;

    std::cout << "Difference\n"
              << std::showpos << std::fixed << std::setprecision(1)
              << "  Average frame time: " << averageDifference << "%\n"
              << "  Throughput:         " << throughputDifference << "%\n"
              << std::noshowpos;

    return 0;
}

} // namespace fluid::benchmark
