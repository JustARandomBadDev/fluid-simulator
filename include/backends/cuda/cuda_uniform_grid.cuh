#ifndef FLUID_BACKENDS_CUDA_CUDA_UNIFORM_GRID_CUH
#define FLUID_BACKENDS_CUDA_CUDA_UNIFORM_GRID_CUH

#include <vector_functions.h>
#include <vector_types.h>

namespace fluid::simulation {

__device__ __forceinline__ int3 positionToCell(
    float3 p_position,
    float3 p_grid_position,
    float p_inv_cell_size
) {
    return make_int3(
        static_cast<int>(
            (p_position.x - p_grid_position.x) * p_inv_cell_size
        ),
        static_cast<int>(
            (p_position.y - p_grid_position.y) * p_inv_cell_size
        ),
        static_cast<int>((p_position.z - p_grid_position.z) * p_inv_cell_size)
    );
}

__device__ __forceinline__ uint32_t
getCellIndex(uint3 p_position, uint3 p_dimensions) {
    return p_position.x + p_position.y * p_dimensions.x +
           p_position.z * p_dimensions.x * p_dimensions.y;
}

__device__ __forceinline__ bool
isCellInsideGrid(int3 p_position, uint3 p_dimensions) {
    return p_position.x >= 0 && p_position.y >= 0 && p_position.z >= 0 &&
           p_position.x < static_cast<int>(p_dimensions.x) &&
           p_position.y < static_cast<int>(p_dimensions.y) &&
           p_position.z < static_cast<int>(p_dimensions.z);
}

}

#endif
