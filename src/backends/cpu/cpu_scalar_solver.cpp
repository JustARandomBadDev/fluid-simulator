#include "backends/cpu/cpu_scalar_solver.hpp"
#include "simulation/particle_system.hpp"
#include "simulation/particles_data.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include <glm/glm.hpp>

namespace fluid::simulation {

void CpuScalarSolver::init(glm::vec3 p_box_dim) {
    _box_dim = p_box_dim;

    _grid.init(
        p_box_dim,
        _params.smoothingRadius,
        ParticleSystem::MAX_PARTICLES
    );
}

void CpuScalarSolver::step(
    ParticleData& p_particles,
    float p_dt
) {
    std::cout << p_dt << "\n";

    p_dt = std::min(p_dt, 0.001f);

    const glm::vec3 gravity{0.0f, -9.81f, 0.0f};

    _grid.rebuild(p_particles);

    // Density
    #pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < p_particles.count; i++) {
        const auto& pi = p_particles.positions[i];
        const glm::ivec3 cell_pos = _grid.positionToCell(pi);

        float density = 0.0f;

        for (int z = -1; z <= 1; z++) {
            for (int y = -1; y <= 1; y++) {
                for (int x = -1; x <= 1; x++) {
                    const glm::ivec3 neighbor_pos =
                        cell_pos + glm::ivec3(x, y, z);

                    if (!_grid.contains(neighbor_pos))
                        continue;

                    const uint32_t cell_index =
                        _grid.get(glm::uvec3(neighbor_pos));

                    const uint32_t offset =
                        _grid.getOffset(cell_index);

                    const uint32_t count =
                        _grid.getCount(cell_index);

                    for (
                        uint32_t k = offset;
                        k < offset + count;
                        k++
                    ) {
                        const uint32_t j =
                            _grid.getParticleIndex(k);

                        const auto& pj =
                            p_particles.positions[j];

                        const glm::vec3 rij = pi - pj;
                        const float r2 = glm::dot(rij, rij);

                        if (r2 < _constants.h2) {
                            const float q =
                                _constants.h2 - r2;

                            density +=
                                _params.particleMass *
                                _constants.poly6 *
                                q * q * q;
                        }
                    }
                }
            }
        }

        p_particles.densities[i] = density;
    }

    // Pressure
    #pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < p_particles.count; i++) {
        p_particles.pressures[i] = std::max(
            _params.stiffness *
                (
                    p_particles.densities[i] -
                    _params.restDensity
                ),
            0.0f
        );
    }

    // Accelerations
    std::vector<glm::vec3> accelerations(
        p_particles.count,
        glm::vec3{0.0f}
    );

    #pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < p_particles.count; i++) {
        const auto& po_i = p_particles.positions[i];
        const auto& ve_i = p_particles.velocities[i];
        const auto& de_i = p_particles.densities[i];
        const auto& pr_i = p_particles.pressures[i];

        const glm::ivec3 cell_pos =
            _grid.positionToCell(po_i);

        glm::vec3 pressureAcceleration{0.0f};
        glm::vec3 viscosityAcceleration{0.0f};

        for (int z = -1; z <= 1; z++) {
            for (int y = -1; y <= 1; y++) {
                for (int x = -1; x <= 1; x++) {
                    const glm::ivec3 neighbor_pos =
                        cell_pos + glm::ivec3(x, y, z);

                    if (!_grid.contains(neighbor_pos))
                        continue;

                    const uint32_t cell_index =
                        _grid.get(glm::uvec3(neighbor_pos));

                    const uint32_t offset =
                        _grid.getOffset(cell_index);

                    const uint32_t count =
                        _grid.getCount(cell_index);

                    for (
                        uint32_t k = offset;
                        k < offset + count;
                        k++
                    ) {
                        const uint32_t j =
                            _grid.getParticleIndex(k);

                        if (i == j)
                            continue;

                        const auto& po_j =
                            p_particles.positions[j];

                        const auto& ve_j =
                            p_particles.velocities[j];

                        const auto& de_j =
                            p_particles.densities[j];

                        const auto& pr_j =
                            p_particles.pressures[j];

                        const glm::vec3 rij = po_i - po_j;
                        const float r2 = glm::dot(rij, rij);

                        if (
                            r2 <= 0.0f ||
                            r2 >= _constants.h2
                        ) {
                            continue;
                        }

                        const float r = std::sqrt(r2);

                        const glm::vec3 direction =
                            rij / r;

                        const float distanceToEdge =
                            _params.smoothingRadius - r;

                        // Pressure
                        const glm::vec3 gradW =
                            -_constants.spiky *
                            distanceToEdge *
                            distanceToEdge *
                            direction;

                        pressureAcceleration -=
                            _params.particleMass *
                            (
                                pr_i / (de_i * de_i) +
                                pr_j / (de_j * de_j)
                            ) *
                            gradW;

                        // Viscosity
                        const float laplacian =
                            _constants.spiky *
                            distanceToEdge;

                        viscosityAcceleration +=
                            _params.viscosity *
                            _params.particleMass *
                            (ve_j - ve_i) /
                            de_j *
                            laplacian;
                    }
                }
            }
        }

        accelerations[i] =
            pressureAcceleration +
            viscosityAcceleration +
            gravity;
    }

    // Integration and box collision
    #pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < p_particles.count; i++) {
        auto& position = p_particles.positions[i];
        auto& velocity = p_particles.velocities[i];

        velocity += accelerations[i] * p_dt;
        position += velocity * p_dt;

        if (_box_dim.y > 0.0f) {
            if (position.y < 0.0f) {
                position.y = 0.0f;
                velocity.y = 0.0f;
            } else if (position.y > _box_dim.y) {
                position.y = _box_dim.y;
                velocity.y = 0.0f;
            }
        }

        if (_box_dim.x > 0.0f) {
            if (position.x < 0.0f) {
                position.x = 0.0f;
                velocity.x = 0.0f;
            } else if (position.x > _box_dim.x) {
                position.x = _box_dim.x;
                velocity.x = 0.0f;
            }
        }

        if (_box_dim.z > 0.0f) {
            if (position.z < 0.0f) {
                position.z = 0.0f;
                velocity.z = 0.0f;
            } else if (position.z > _box_dim.z) {
                position.z = _box_dim.z;
                velocity.z = 0.0f;
            }
        }
    }
}

}