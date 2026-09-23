#ifndef FLUID_GRAPHICS_VULKAN_PARTICLE_BUFFER_HPP
#define FLUID_GRAPHICS_VULKAN_PARTICLE_BUFFER_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "graphics/vulkan/particle_vertex.hpp"
#include "graphics/vulkan/vulkan_buffer.hpp"

namespace fluid::graphics {

class Device;

class ParticleBuffer {
  public:
    explicit ParticleBuffer(Device &p_device);

    void initialize(std::size_t p_capacity, uint32_t p_frames_in_flight);

    void
    update(uint32_t p_frame_index, std::span<const ParticleVertex> p_particles);

    void cleanup();

    VulkanBuffer &buffer(uint32_t p_frame_index);

    const VulkanBuffer &buffer(uint32_t p_frame_index) const;

    uint32_t count(uint32_t p_frame_index) const;

  private:
    Device &_device;

    std::vector<VulkanBuffer> _buffers;
    std::vector<void *> _mapped_buffers;
    std::vector<uint32_t> _counts;

    std::size_t _capacity = 0;
};

} // namespace fluid::graphics

#endif
