#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <cuda_runtime_api.h>

#include "app/application.hpp"
#include "benchmark/benchmark.hpp"
#include "benchmark/interop_benchmark.hpp"
#include "config.hpp"

namespace {

struct CommandLineOptions {
    bool help = false;
    bool benchmark = false;
    bool benchmarkInterop = false;
    bool smokeTest = false;
    bool enableInterop = false;
    fluid::SimulationBackend backend = fluid::SimulationBackend::Cuda;
};

void printHelp() {
    std::cout << "Fluid Simulator\n\n"
                 "Usage:\n"
                 "  ./fluid-simulator [options]\n\n"
                 "Options:\n"
                 "  -h, --help          Show this help message\n"
                 "  --backend cpu       Use the CPU scalar backend\n"
                 "  --backend cuda      Use the CUDA backend (default)\n"
                 "  --benchmark         Run the headless simulation benchmark\n"
                 "  --benchmark-interop Compare CUDA rendering paths\n"
                 "  --interop           Enable Vulkan/CUDA interoperability\n"
                 "  --smoke-test        Run the existing graphics smoke test\n";
}

fluid::SimulationBackend parseBackend(std::string_view value) {
    if (value == "cpu") return fluid::SimulationBackend::CpuScalar;
    if (value == "cuda") return fluid::SimulationBackend::Cuda;

    throw std::runtime_error(
        "invalid backend '" + std::string(value) + "' (expected cpu or cuda)"
    );
}

CommandLineOptions parseCommandLine(int argc, char** argv) {
    CommandLineOptions options;

    for (int index = 1; index < argc; index++) {
        const std::string_view argument = argv[index];

        if (argument == "-h" || argument == "--help") {
            options.help = true;
        } else if (argument == "--benchmark") {
            options.benchmark = true;
        } else if (argument == "--benchmark-interop") {
            options.benchmarkInterop = true;
        } else if (argument == "--smoke-test") {
            options.smokeTest = true;
        } else if (argument == "--interop") {
            options.enableInterop = true;
        } else if (argument == "--backend") {
            if (index + 1 >= argc) {
                throw std::runtime_error("--backend requires cpu or cuda");
            }
            index++;
            options.backend = parseBackend(argv[index]);
        } else {
            throw std::runtime_error(
                "unknown argument '" + std::string(argument) + "'"
            );
        }
    }

    if (!options.help && options.benchmark && options.smokeTest) {
        throw std::runtime_error(
            "--benchmark cannot be combined with --smoke-test"
        );
    }
    if (!options.help && options.benchmark && options.benchmarkInterop) {
        throw std::runtime_error(
            "--benchmark cannot be combined with --benchmark-interop"
        );
    }
    if (!options.help && options.benchmarkInterop && options.smokeTest) {
        throw std::runtime_error(
            "--benchmark-interop cannot be combined with --smoke-test"
        );
    }

    return options;
}

void validateBackendAvailability(fluid::SimulationBackend backend) {
    if (backend != fluid::SimulationBackend::Cuda) return;

    int deviceCount = 0;
    const cudaError_t result = cudaGetDeviceCount(&deviceCount);
    if (result != cudaSuccess) {
        throw std::runtime_error(
            "CUDA backend unavailable: " +
            std::string(cudaGetErrorString(result))
        );
    }
    if (deviceCount == 0) {
        throw std::runtime_error("CUDA backend unavailable: no device found");
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        const CommandLineOptions options = parseCommandLine(argc, argv);
        if (options.help) {
            printHelp();
            return 0;
        }

        if (options.benchmarkInterop) {
            validateBackendAvailability(fluid::SimulationBackend::Cuda);
            return fluid::benchmark::runInterop();
        }

        validateBackendAvailability(options.backend);

        if (options.benchmark) {
            fluid::benchmark::BenchmarkConfig config;
            config.simulation.backend = options.backend;
            return fluid::benchmark::run(config);
        }

        fluid::ApplicationConfig config;
        config.simulation.backend = options.backend;
        config.graphics.enableCudaInterop = options.enableInterop;

        fluid::Application application(std::move(config));
        application.run(options.smokeTest);
    } catch (const std::exception& exception) {
        std::cerr << "fluid-simulator: " << exception.what() << '\n';
        return 1;
    }

    return 0;
}
