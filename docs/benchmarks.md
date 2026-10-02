# Benchmark History

[Back to the README](../README.md)

This document records chronological end-to-end simulation measurements. New
implementation milestones should append results instead of replacing the
baseline so that later CUDA grid, transfer, and interoperability work can be
compared after each change is implemented.

## Methodology

The headless benchmark uses the real `FluidSimulator` and selected backend.
Warmup steps are excluded from the measurement, which times complete simulation
updates with `std::chrono::steady_clock`. CUDA timings include host-to-device
copies, the three simulation kernels, device synchronization, and
device-to-host copies performed by `CudaSolver::step()`.

Both backends use the same default particle state, scene, SPH parameters, time
step, and number of iterations. No backend-specific tuning is applied.

## Build configuration

Benchmarks were built from the repository root with:

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS_RELEASE="-O3 -march=native -DNDEBUG"

cmake --build build -j6
```

`-O3` enables aggressive compiler optimization, `-march=native` enables
instructions and optimizations for the host CPU, and `-DNDEBUG` disables debug
assertions. The CPU solver contains no explicit SIMD intrinsics; GCC may
auto-vectorize suitable code under `-O3 -march=native`. CPU results are
therefore specific to this machine and compiler target, and the resulting
binary does not represent a portable generic CPU build.

All future performance comparisons should use the same compiler and build
configuration unless that configuration is intentionally being benchmarked.
If the flags change, the new flags must be recorded with that milestone.

The benchmark commands are:

```sh
./build/fluid-simulator --benchmark --backend cpu
./build/fluid-simulator --benchmark --backend cuda
```

## Test system

- CPU: Intel Core i5-9600K @ 3.70 GHz
- GPU: NVIDIA GeForce RTX 2060
- OS: Arch Linux, Linux 7.2.7-arch1-1
- C++ compiler: GCC 16.2.1
- CUDA compiler: CUDA 13.4 (`nvcc` V13.4.92)
- Build type: Release
- C++ Release flags: `-O3 -march=native -DNDEBUG`
- OpenMP: default runtime settings, 6 logical CPUs available
  (`OMP_NUM_THREADS` unset)

## Results

### Baseline — CPU Uniform Grid / naive CUDA all-pairs

- Date: 2026-10-02
- Git: `6be862e` (working tree clean at measurement time)
- Build configuration: Release with `-O3 -march=native -DNDEBUG`
- Active particles: 20,000; capacity: 100,000
- Warmup steps: 100; measured steps: 1,000; delta time: 0.001 s
- Simulation box: position `(0, 0, 0)`, dimensions `(2, 20, 2)`
- Spawn box: position `(0.5, 0.5, 0.5)`, dimensions `(1, 2.5, 1)`
- Initial velocity: `(0, 0, 0)`; gravity: `(0, -9.81, 0)`
- SPH: mass `0.12`, radius `0.1`, rest density `1000`, stiffness `40`,
  viscosity `0.03`
- CUDA block size: 256 threads

| Backend    | Particles | Total time |  Avg step | Steps/s |  Particles/s |
| ---------- | --------: | ---------: | --------: | ------: | -----------: |
| CPU Scalar |    20,000 |   10.441 s | 10.441 ms |   95.77 | 1,915,487.53 |
| CUDA       |    20,000 |    5.585 s |  5.585 ms |  179.07 | 3,581,322.28 |

Notes:

- The CPU scalar backend uses OpenMP and a flat Uniform Grid for neighbor
  lookup.
- The CUDA backend launches one thread per particle and uses an O(n²)
  all-pairs neighbor search. Device allocations persist, while position and
  velocity transfers and synchronization remain inside every measured step.
