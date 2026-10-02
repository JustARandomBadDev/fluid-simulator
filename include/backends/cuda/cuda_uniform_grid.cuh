#ifndef FLUID_BACKENDS_CUDA_CUDA_UNIFORM_GRID_CUH
#define FLUID_BACKENDS_CUDA_CUDA_UNIFORM_GRID_CUH

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_uint3.hpp>
#include <glm/ext/vector_int3.hpp>

namespace fluid::simulation {

__device__ __forceinline__ glm::ivec3 positionToCell(
    glm::vec3 p_position,
    glm::vec3 p_grid_position,
    float p_inv_cell_size
) {
    return glm::ivec3(
    static_cast<int>((p_position.x - p_grid_position.x) * p_inv_cell_size),
    static_cast<int>((p_position.y - p_grid_position.y) * p_inv_cell_size),
    static_cast<int>((p_position.z - p_grid_position.z) * p_inv_cell_size)
);
}

__device__ __forceinline__ uint32_t
get(glm::uvec3 p_position, glm::uvec3 p_dimensions) {
    return p_position.x + p_position.y * p_dimensions.x +
           p_position.z * p_dimensions.x * p_dimensions.y;
}

__device__ __forceinline__ bool
contain(glm::ivec3 p_position, glm::uvec3 p_dimensions) {
    return p_position.x >= 0 && p_position.y >= 0 && p_position.z >= 0 &&
           p_position.x < static_cast<int>(p_dimensions.x) &&
           p_position.y < static_cast<int>(p_dimensions.y) &&
           p_position.z < static_cast<int>(p_dimensions.z);
}

}

#endif