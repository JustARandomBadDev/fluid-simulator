#ifndef FLUID_GRAPHICS_VULKAN_PARTICLE_VERTEX_HPP
#define FLUID_GRAPHICS_VULKAN_PARTICLE_VERTEX_HPP

#include <array>
#include <cstddef>

#include <vulkan/vulkan.h>

namespace fluid::graphics {

struct ParticleVertex {
    float x;
    float y;
    float z;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription description{};
        description.binding = 0;
        description.stride = sizeof(ParticleVertex);
        description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return description;
    }

    static std::array<VkVertexInputAttributeDescription, 1>
    getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 1> descriptions{};
        descriptions[0].binding = 0;
        descriptions[0].location = 0;
        descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        descriptions[0].offset = offsetof(ParticleVertex, x);
        return descriptions;
    }
};

} // namespace fluid::graphics

#endif
