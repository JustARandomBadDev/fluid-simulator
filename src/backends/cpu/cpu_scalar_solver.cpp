#include "backends/cpu/cpu_scalar_solver.hpp"

#include "simulation/particle_system.hpp"
#include "simulation/particles_data.hpp"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>

namespace fluid::simulation {

void CpuScalarSolver::init(glm::vec3 p_box_dim) {
    _box_dim = p_box_dim;

    _grid.init(
        p_box_dim,
        _params.smoothingRadius,
        ParticleSystem::MAX_PARTICLES
    );

    _accelerations.resize(
        ParticleSystem::MAX_PARTICLES
    );

    _inverse_densities.resize(
        ParticleSystem::MAX_PARTICLES
    );

    _pressure_terms.resize(
        ParticleSystem::MAX_PARTICLES
    );
}

void CpuScalarSolver::step(
    ParticleData& p_particles,
    float p_dt
) {
    if (p_particles.count == 0)
        return;

    p_dt = std::min(p_dt, 0.001f);

    const glm::vec3 gravity{
        0.0f,
        -9.81f,
        0.0f
    };

    const float density_factor =
        _params.particleMass *
        _constants.poly6;

    const float pressure_factor =
        _params.particleMass *
        _constants.spiky;

    const float viscosity_factor =
        _params.viscosity *
        _params.particleMass *
        _constants.spiky;

    _grid.rebuild(p_particles);

    #pragma omp parallel
    {
        // -----------------------------------------------------
        // Density + pressure
        // -----------------------------------------------------

        #pragma omp for schedule(static)
        for (
            std::size_t i = 0;
            i < p_particles.count;
            i++
        ) {
            const auto& position_i =
                p_particles.positions[i];

            const float pix = position_i.x;
            const float piy = position_i.y;
            const float piz = position_i.z;

            const glm::ivec3& cell_pos =
                _grid.getParticleCellPosition(i);

            float density = 0.0f;

            for (int z = -1; z <= 1; z++) {
                for (int y = -1; y <= 1; y++) {
                    for (int x = -1; x <= 1; x++) {
                        const glm::ivec3 neighbor_pos =
                            cell_pos +
                            glm::ivec3(x, y, z);

                        if (!_grid.contains(neighbor_pos))
                            continue;

                        const uint32_t cell_index =
                            _grid.get(
                                glm::uvec3(neighbor_pos)
                            );

                        const uint32_t offset =
                            _grid.getOffset(cell_index);

                        const uint32_t count =
                            _grid.getCount(cell_index);

                        const uint32_t end =
                            offset + count;

                        for (
                            uint32_t k = offset;
                            k < end;
                            k++
                        ) {
                            const uint32_t j =
                                _grid.getParticleIndex(k);

                            const auto& position_j =
                                p_particles.positions[j];

                            const float rx =
                                pix - position_j.x;

                            const float ry =
                                piy - position_j.y;

                            const float rz =
                                piz - position_j.z;

                            const float r2 =
                                rx * rx +
                                ry * ry +
                                rz * rz;

                            if (r2 >= _constants.h2)
                                continue;

                            const float q =
                                _constants.h2 - r2;

                            density +=
                                density_factor *
                                q * q * q;
                        }
                    }
                }
            }

            p_particles.densities[i] =
                density;

            const float pressure =
                std::max(
                    _params.stiffness *
                        (
                            density -
                            _params.restDensity
                        ),
                    0.0f
                );

            p_particles.pressures[i] =
                pressure;

            const float safe_density =
                std::max(
                    density,
                    1e-6f
                );

            const float inverse_density =
                1.0f / safe_density;

            _inverse_densities[i] =
                inverse_density;

            _pressure_terms[i] =
                pressure *
                inverse_density *
                inverse_density;
        }

        // -----------------------------------------------------
        // Pressure + viscosity acceleration
        // -----------------------------------------------------

        #pragma omp for schedule(static)
        for (
            std::size_t i = 0;
            i < p_particles.count;
            i++
        ) {
            const auto& position_i =
                p_particles.positions[i];

            const auto& velocity_i =
                p_particles.velocities[i];

            const float pix = position_i.x;
            const float piy = position_i.y;
            const float piz = position_i.z;

            const float vix = velocity_i.x;
            const float viy = velocity_i.y;
            const float viz = velocity_i.z;

            const glm::ivec3& cell_pos =
                _grid.getParticleCellPosition(i);

            const float pressure_i =
                _pressure_terms[i];

            float pressure_x = 0.0f;
            float pressure_y = 0.0f;
            float pressure_z = 0.0f;

            float viscosity_x = 0.0f;
            float viscosity_y = 0.0f;
            float viscosity_z = 0.0f;

            for (int z = -1; z <= 1; z++) {
                for (int y = -1; y <= 1; y++) {
                    for (int x = -1; x <= 1; x++) {
                        const glm::ivec3 neighbor_pos =
                            cell_pos +
                            glm::ivec3(x, y, z);

                        if (!_grid.contains(neighbor_pos))
                            continue;

                        const uint32_t cell_index =
                            _grid.get(
                                glm::uvec3(neighbor_pos)
                            );

                        const uint32_t offset =
                            _grid.getOffset(cell_index);

                        const uint32_t count =
                            _grid.getCount(cell_index);

                        const uint32_t end =
                            offset + count;

                        for (
                            uint32_t k = offset;
                            k < end;
                            k++
                        ) {
                            const uint32_t j =
                                _grid.getParticleIndex(k);

                            if (i == j)
                                continue;

                            const auto& position_j =
                                p_particles.positions[j];

                            const auto& velocity_j =
                                p_particles.velocities[j];

                            const float rx =
                                pix - position_j.x;

                            const float ry =
                                piy - position_j.y;

                            const float rz =
                                piz - position_j.z;

                            const float r2 =
                                rx * rx +
                                ry * ry +
                                rz * rz;

                            if (
                                r2 <= 0.0f ||
                                r2 >= _constants.h2
                            ) {
                                continue;
                            }

                            const float r =
                                std::sqrt(r2);

                            const float inverse_r =
                                1.0f / r;

                            const float distance_to_edge =
                                _params.smoothingRadius -
                                r;

                            const float distance_to_edge2 =
                                distance_to_edge *
                                distance_to_edge;

                            // Pressure
                            const float pressure_scalar =
                                pressure_factor *
                                (
                                    pressure_i +
                                    _pressure_terms[j]
                                ) *
                                distance_to_edge2 *
                                inverse_r;

                            pressure_x +=
                                rx * pressure_scalar;

                            pressure_y +=
                                ry * pressure_scalar;

                            pressure_z +=
                                rz * pressure_scalar;

                            // Viscosity
                            const float viscosity_scalar =
                                viscosity_factor *
                                _inverse_densities[j] *
                                distance_to_edge;

                            viscosity_x +=
                                (
                                    velocity_j.x -
                                    vix
                                ) *
                                viscosity_scalar;

                            viscosity_y +=
                                (
                                    velocity_j.y -
                                    viy
                                ) *
                                viscosity_scalar;

                            viscosity_z +=
                                (
                                    velocity_j.z -
                                    viz
                                ) *
                                viscosity_scalar;
                        }
                    }
                }
            }

            _accelerations[i] = {
                pressure_x +
                    viscosity_x +
                    gravity.x,

                pressure_y +
                    viscosity_y +
                    gravity.y,

                pressure_z +
                    viscosity_z +
                    gravity.z
            };
        }

        // Implicit barrier:
        // all accelerations use the old particle state.

        // -----------------------------------------------------
        // Integration + box collision
        // -----------------------------------------------------

        #pragma omp for schedule(static)
        for (
            std::size_t i = 0;
            i < p_particles.count;
            i++
        ) {
            auto& position =
                p_particles.positions[i];

            auto& velocity =
                p_particles.velocities[i];

            const auto& acceleration =
                _accelerations[i];

            velocity.x +=
                acceleration.x * p_dt;

            velocity.y +=
                acceleration.y * p_dt;

            velocity.z +=
                acceleration.z * p_dt;

            position.x +=
                velocity.x * p_dt;

            position.y +=
                velocity.y * p_dt;

            position.z +=
                velocity.z * p_dt;

            // Y
            if (_box_dim.y > 0.0f) {
                if (position.y < 0.0f) {
                    position.y = 0.0f;
                    velocity.y = 0.0f;
                } else if (
                    position.y > _box_dim.y
                ) {
                    position.y =
                        _box_dim.y;

                    velocity.y = 0.0f;
                }
            }

            // X
            if (_box_dim.x > 0.0f) {
                if (position.x < 0.0f) {
                    position.x = 0.0f;
                    velocity.x = 0.0f;
                } else if (
                    position.x > _box_dim.x
                ) {
                    position.x =
                        _box_dim.x;

                    velocity.x = 0.0f;
                }
            }

            // Z
            if (_box_dim.z > 0.0f) {
                if (position.z < 0.0f) {
                    position.z = 0.0f;
                    velocity.z = 0.0f;
                } else if (
                    position.z > _box_dim.z
                ) {
                    position.z =
                        _box_dim.z;

                    velocity.z = 0.0f;
                }
            }
        }
    }
}

}