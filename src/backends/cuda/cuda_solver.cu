#include "backends/cuda/cuda_solver.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cuda_runtime.h>
#include <stdexcept>
#include <string>
#include <unistd.h>

#include "graphics/vulkan/particle_vertex.hpp"

namespace fluid::simulation {
namespace {

__global__ void writeParticleVerticesKernel(
    const float* p_position_x,
    const float* p_position_y,
    const float* p_position_z,
    graphics::ParticleVertex* p_render_buffer,
    std::size_t p_particle_count
) {
    const std::size_t index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index >= p_particle_count) return;

    p_render_buffer[index].x = p_position_x[index];
    p_render_buffer[index].y = p_position_y[index];
    p_render_buffer[index].z = p_position_z[index];
}

void throwIfCudaInteropFailed(cudaError_t p_result, const char* p_operation) {
    if (p_result == cudaSuccess) return;

    throw std::runtime_error(
        std::string(p_operation) + " -> " + cudaGetErrorString(p_result)
    );
}

} // namespace

void CudaSolver::init() {
    _uniform_grid.init(_params.smoothingRadius);

    const std::size_t buffer_size = _config.particleCapacity * sizeof(float);

    cudaMalloc(&_position_x, buffer_size);
    cudaMalloc(&_position_y, buffer_size);
    cudaMalloc(&_position_z, buffer_size);

    cudaMalloc(&_velocity_x, buffer_size);
    cudaMalloc(&_velocity_y, buffer_size);
    cudaMalloc(&_velocity_z, buffer_size);

    cudaMalloc(&_inverse_densities, buffer_size);
    cudaMalloc(&_pressure_terms, buffer_size);

    cudaMalloc(&_accelerations_x, buffer_size);
    cudaMalloc(&_accelerations_y, buffer_size);
    cudaMalloc(&_accelerations_z, buffer_size);
}

void CudaSolver::configureInterop(std::size_t p_slot_count) {
    if (p_slot_count == 0) {
        throw std::runtime_error(
            "CudaSolver::configureInterop() -> slot count must be non-zero"
        );
    }

    cleanupInterop();
    _interop_data.resize(p_slot_count);
    _device_state_initialized = false;
}

void CudaSolver::importRenderBuffer(
    int p_fd,
    std::size_t p_allocation_size,
    std::size_t p_buffer_size,
    std::size_t p_frame
) {
    if (p_frame >= _interop_data.size()) {
        ::close(p_fd);
        throw std::runtime_error(
            "CudaSolver::importRenderBuffer() -> invalid frame slot"
        );
    }

    CudaInteropData& interop = _interop_data[p_frame];
    if (interop.external_memory != nullptr ||
        interop.render_buffer != nullptr) {
        ::close(p_fd);
        throw std::runtime_error(
            "CudaSolver::importRenderBuffer() -> frame slot already imported"
        );
    }

    cudaExternalMemoryHandleDesc memory_desc{};
    memory_desc.type = cudaExternalMemoryHandleTypeOpaqueFd;
    memory_desc.handle.fd = p_fd;
    memory_desc.size = p_allocation_size;
    memory_desc.flags = cudaExternalMemoryDedicated;

    const cudaError_t import_result =
        cudaImportExternalMemory(&interop.external_memory, &memory_desc);
    if (import_result != cudaSuccess) {
        ::close(p_fd);
        throw std::runtime_error(
            "CudaSolver::importRenderBuffer() -> " +
            std::string(cudaGetErrorString(import_result))
        );
    }

    cudaExternalMemoryBufferDesc buffer_desc{};
    buffer_desc.offset = 0;
    buffer_desc.size = p_buffer_size;
    buffer_desc.flags = 0;

    const cudaError_t map_result = cudaExternalMemoryGetMappedBuffer(
        reinterpret_cast<void**>(&interop.render_buffer),
        interop.external_memory,
        &buffer_desc
    );
    if (map_result != cudaSuccess) {
        cudaDestroyExternalMemory(interop.external_memory);
        interop.external_memory = nullptr;
        throw std::runtime_error(
            "CudaSolver::importRenderBuffer() -> " +
            std::string(cudaGetErrorString(map_result))
        );
    }
}

void CudaSolver::importCudaSemaphore(int p_fd, std::size_t p_frame) {
    if (p_frame >= _interop_data.size()) {
        ::close(p_fd);
        throw std::runtime_error(
            "CudaSolver::importCudaSemaphore() -> invalid frame slot"
        );
    }

    CudaInteropData& interop = _interop_data[p_frame];
    if (interop.semaphore != nullptr) {
        ::close(p_fd);
        throw std::runtime_error(
            "CudaSolver::importCudaSemaphore() -> frame slot already imported"
        );
    }

    cudaExternalSemaphoreHandleDesc desc{};
    desc.type = cudaExternalSemaphoreHandleTypeTimelineSemaphoreFd;
    desc.handle.fd = p_fd;
    desc.flags = 0;

    const cudaError_t result =
        cudaImportExternalSemaphore(&interop.semaphore, &desc);

    if (result != cudaSuccess) {
        ::close(p_fd);
        throw std::runtime_error(
            "CudaSolver::importCudaSemaphore() -> " +
            std::string(cudaGetErrorString(result))
        );
    }
}

void CudaSolver::step(ParticleData& p_particles, float p_dt) {
    _particle_count = p_particles.count;
    if (_particle_count == 0) return;

    p_dt = std::min(p_dt, _config.maximumTimeStep);

    const std::size_t size = p_particles.count * sizeof(float);

    if (!hasInterop() || !_device_state_initialized) {
        cudaMemcpy(
            _position_x,
            p_particles.position_x.data(),
            size,
            cudaMemcpyHostToDevice
        );
        cudaMemcpy(
            _position_y,
            p_particles.position_y.data(),
            size,
            cudaMemcpyHostToDevice
        );
        cudaMemcpy(
            _position_z,
            p_particles.position_z.data(),
            size,
            cudaMemcpyHostToDevice
        );
        cudaMemcpy(
            _velocity_x,
            p_particles.velocity_x.data(),
            size,
            cudaMemcpyHostToDevice
        );
        cudaMemcpy(
            _velocity_y,
            p_particles.velocity_y.data(),
            size,
            cudaMemcpyHostToDevice
        );
        cudaMemcpy(
            _velocity_z,
            p_particles.velocity_z.data(),
            size,
            cudaMemcpyHostToDevice
        );
        _device_state_initialized = true;
    }

    _uniform_grid
        .rebuild(_position_x, _position_y, _position_z, p_particles.count);

    const std::size_t threads = _config.cuda.blockSize;

    const std::size_t blocks = (p_particles.count + threads - 1) / threads;

    const CudaParticleBuffers particles{.position_x = _position_x,
        .position_y = _position_y,
        .position_z = _position_z,
        .velocity_x = _velocity_x,
        .velocity_y = _velocity_y,
        .velocity_z = _velocity_z,
        .count = p_particles.count};

    const CudaSphBuffers sph{.inverse_densities = _inverse_densities,
        .pressure_terms = _pressure_terms,
        .acceleration_x = _accelerations_x,
        .acceleration_y = _accelerations_y,
        .acceleration_z = _accelerations_z};

    const CudaGridView grid = _uniform_grid.getView();

    const CudaSphContext context{.params = _params,
        .constants = _constants,
        .gravity = make_float3(
            _config.gravity.x,
            _config.gravity.y,
            _config.gravity.z
        )};

    pressureAndDensityKernel<<<blocks, threads>>>(
        particles,
        sph,
        grid,
        context
    );

    if (cudaGetLastError() != cudaSuccess) {
        std::printf("CUDA error: pressureAndDensityKernel\n");
    }

    accelerationKernel<<<blocks, threads>>>(particles, sph, grid, context);

    if (cudaGetLastError() != cudaSuccess) {
        std::printf("CUDA error: accelerationKernel\n");
    }

    integrateKernel<<<blocks, threads>>>(particles, sph, _box_config, p_dt);

    if (cudaGetLastError() != cudaSuccess) {
        std::printf("CUDA error: integrateKernel\n");
    }

    if (hasInterop()) return;

    cudaMemcpy(
        p_particles.position_x.data(),
        _position_x,
        size,
        cudaMemcpyDeviceToHost
    );
    cudaMemcpy(
        p_particles.position_y.data(),
        _position_y,
        size,
        cudaMemcpyDeviceToHost
    );
    cudaMemcpy(
        p_particles.position_z.data(),
        _position_z,
        size,
        cudaMemcpyDeviceToHost
    );
    cudaMemcpy(
        p_particles.velocity_x.data(),
        _velocity_x,
        size,
        cudaMemcpyDeviceToHost
    );
    cudaMemcpy(
        p_particles.velocity_y.data(),
        _velocity_y,
        size,
        cudaMemcpyDeviceToHost
    );
    cudaMemcpy(
        p_particles.velocity_z.data(),
        _velocity_z,
        size,
        cudaMemcpyDeviceToHost
    );
}

void CudaSolver::writeRenderBuffer(
    std::size_t p_interop_slot,
    uint64_t& p_cuda_ready_value,
    uint64_t& p_vulkan_done_value
) {
    if (p_interop_slot >= _interop_data.size()) {
        throw std::runtime_error(
            "CudaSolver::writeRenderBuffer() -> invalid interop slot"
        );
    }

    CudaInteropData& interop = _interop_data[p_interop_slot];
    if (interop.render_buffer == nullptr || interop.semaphore == nullptr) {
        throw std::runtime_error(
            "CudaSolver::writeRenderBuffer() -> incomplete interop slot"
        );
    }

    cudaExternalSemaphoreWaitParams waitParams{};
    waitParams.params.fence.value = interop.syncValue;
    throwIfCudaInteropFailed(
        cudaWaitExternalSemaphoresAsync(
            &interop.semaphore,
            &waitParams,
            1,
            nullptr
        ),
        "CudaSolver::writeRenderBuffer() CUDA wait"
    );

    if (_particle_count > 0) {
        const std::size_t threads = _config.cuda.blockSize;
        const std::size_t blocks = (_particle_count + threads - 1) / threads;
        writeParticleVerticesKernel<<<blocks, threads>>>(
            _position_x,
            _position_y,
            _position_z,
            interop.render_buffer,
            _particle_count
        );
        throwIfCudaInteropFailed(
            cudaGetLastError(),
            "CudaSolver::writeRenderBuffer() packing kernel"
        );
    }

    p_cuda_ready_value = interop.syncValue + 1;
    p_vulkan_done_value = interop.syncValue + 2;

    cudaExternalSemaphoreSignalParams signalParams{};
    signalParams.params.fence.value = p_cuda_ready_value;
    throwIfCudaInteropFailed(
        cudaSignalExternalSemaphoresAsync(
            &interop.semaphore,
            &signalParams,
            1,
            nullptr
        ),
        "CudaSolver::writeRenderBuffer() CUDA signal"
    );

    interop.syncValue = p_vulkan_done_value;
}

void CudaSolver::cleanupInterop() {
    for (CudaInteropData& interop : _interop_data) {
        if (interop.render_buffer != nullptr) {
            cudaFree(interop.render_buffer);
            interop.render_buffer = nullptr;
        }
        if (interop.external_memory != nullptr) {
            cudaDestroyExternalMemory(interop.external_memory);
            interop.external_memory = nullptr;
        }
        if (interop.semaphore != nullptr) {
            cudaDestroyExternalSemaphore(interop.semaphore);
            interop.semaphore = nullptr;
        }
    }
    _interop_data.clear();
    _device_state_initialized = false;
}

CudaSolver::~CudaSolver() {
    cleanupInterop();

    cudaFree(_position_x);
    cudaFree(_position_y);
    cudaFree(_position_z);

    cudaFree(_velocity_x);
    cudaFree(_velocity_y);
    cudaFree(_velocity_z);

    cudaFree(_inverse_densities);
    cudaFree(_pressure_terms);

    cudaFree(_accelerations_x);
    cudaFree(_accelerations_y);
    cudaFree(_accelerations_z);
}

} // namespace fluid::simulation
