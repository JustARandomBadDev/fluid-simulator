#include "graphics/vulkan/particle_buffer.hpp"

#include <cstring>
#include <limits>
#include <stdexcept>

#include "graphics/vulkan/device.hpp"

namespace fluid::graphics {

ParticleBuffer::ParticleBuffer(Device& p_device)
: _device(p_device) {}

void ParticleBuffer::initialize(
    std::size_t p_capacity,
    uint32_t p_frames_in_flight
) {
    if (p_capacity == 0 ||
        p_capacity > static_cast<std::size_t>(std::numeric_limits<uint32_t>::max())) {
        throw std::runtime_error("ParticleBuffer::initialize() -> invalid capacity");
    }
    if (p_frames_in_flight == 0) {
        throw std::runtime_error("ParticleBuffer::initialize() -> frames in flight must be non-zero");
    }

    cleanup();
    _buffers.resize(p_frames_in_flight);
    _mapped_buffers.resize(p_frames_in_flight);
    _counts.resize(p_frames_in_flight, 0);

    const VkDeviceSize bufferSize = static_cast<VkDeviceSize>(
        p_capacity * sizeof(ParticleVertex)
    );

    try {
        for (uint32_t frameIndex = 0; frameIndex < p_frames_in_flight; ++frameIndex) {
            _buffers[frameIndex].createBuffer(
                bufferSize,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                _device
            );
            _mapped_buffers[frameIndex] = _buffers[frameIndex].map();
            if (_mapped_buffers[frameIndex] == nullptr) {
                throw std::runtime_error("ParticleBuffer::initialize() -> failed to map particle buffer");
            }
        }
    } catch (...) {
        cleanup();
        throw;
    }

    _capacity = p_capacity;
}

void ParticleBuffer::update(
    uint32_t p_frame_index,
    std::span<const ParticleVertex> p_particles
) {
    if (p_particles.size() > _capacity) {
        throw std::runtime_error("ParticleBuffer::update() -> capacity exceeded");
    }
    if (p_frame_index >= _buffers.size()) {
        throw std::runtime_error("ParticleBuffer::update() -> invalid frame index");
    }

    if (!p_particles.empty()) {
        std::memcpy(
            _mapped_buffers.at(p_frame_index),
            p_particles.data(),
            p_particles.size_bytes()
        );
    }

    _counts[p_frame_index] = static_cast<uint32_t>(p_particles.size());
}

void ParticleBuffer::cleanup() {
    _mapped_buffers.clear();
    _counts.clear();
    _buffers.clear();
    _capacity = 0;
}

VulkanBuffer& ParticleBuffer::buffer(uint32_t p_frame_index) {
    return _buffers.at(p_frame_index);
}

const VulkanBuffer& ParticleBuffer::buffer(uint32_t p_frame_index) const {
    return _buffers.at(p_frame_index);
}

uint32_t ParticleBuffer::count(uint32_t p_frame_index) const {
    return _counts.at(p_frame_index);
}

} // namespace fluid::graphics
