#include "backends/cpu/cpu_scalar_solver.hpp"
#include "simulation/particles_data.hpp"

#include <glm/glm.hpp>
#include <algorithm>
#include <vector>

namespace fluid::simulation {

void CpuScalarSolver::init(glm::vec3 p_box_dim) {
    _box_dim = p_box_dim;
}

void CpuScalarSolver::step(
    ParticleData& p_particles,
    float p_dt
) {
    p_dt = std::min(p_dt, 0.001f);

    const glm::vec3 gravity{0.0f, -9.81f, 0.0f};

    // Density
    for (std::size_t i = 0; i < p_particles.count; i++) {
        const auto& pi = p_particles.positions[i];

        float density = 0.f;

        for (std::size_t j = 0; j < p_particles.count; j++) {
            const auto& pj = p_particles.positions[j];

            const glm::vec3 rij = pi - pj;
            const float r2 = glm::dot(rij, rij);

            if (r2 < _constants.h2) {
                const float x = _constants.h2 - r2;

                density +=
                    _params.particleMass *
                    _constants.poly6 *
                    x * x * x;
            }
        }

        p_particles.densities[i] = density;
    }

    // Pressure
    for (std::size_t i = 0; i < p_particles.count; i++) {
        p_particles.pressures[i] = std::max(
            _params.stiffness * (p_particles.densities[i] - _params.restDensity),
            0.0f
        );
    }

    // Accelerations
    std::vector<glm::vec3> accelerations(
        p_particles.count,
        glm::vec3{0.0f}
    );

    for (std::size_t i = 0; i < p_particles.count; ++i) {
        const auto& po_i = p_particles.positions[i];
        const auto& ve_i = p_particles.velocities[i];
        const auto& de_i = p_particles.densities[i];
        const auto& pr_i = p_particles.pressures[i];

        glm::vec3 pressureAcceleration{0.0f};
        glm::vec3 viscosityAcceleration{0.0f};

        for (std::size_t j = 0; j < p_particles.count; ++j) {
            const auto& po_j = p_particles.positions[j];
            const auto& ve_j = p_particles.velocities[j];
            const auto& de_j = p_particles.densities[j];
            const auto& pr_j = p_particles.pressures[j];

            if (i == j) {
                continue;
            }

            const glm::vec3 rij = po_i - po_j;
            const float r2 = glm::dot(rij, rij);

            if (r2 <= 0.0f || r2 >= _constants.h2)
                continue;

            const float r = std::sqrt(r2);

            if (r <= 0.0f || r >= _params.smoothingRadius) {
                continue;
            }

            const glm::vec3 direction = rij / r;
            const float distanceToEdge = _params.smoothingRadius - r;

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
                _constants.spiky * distanceToEdge;

            viscosityAcceleration +=
                _params.viscosity *
                _params.particleMass *
                (ve_j - ve_i) /
                de_j *
                laplacian;
        }

        accelerations[i] =
            pressureAcceleration +
            viscosityAcceleration +
            gravity;
    }

    // Box collision
    for (std::size_t i = 0; i < p_particles.count; ++i) {
        auto& position = p_particles.positions[i];
        auto& velocity = p_particles.velocities[i];

        velocity += accelerations[i] * p_dt;
        position += velocity * p_dt;

        if (_box_dim.y > 0.f) {
            if (position.y < 0.0f) {
                position.y = 0.0f;
                velocity.y = 0.0f;
            } else if (
                position.y > _box_dim.y) {
                position.y = _box_dim.y;
                velocity.y = 0.0f;
            }
        }

        if (_box_dim.x > 0.f) {
            if (position.x < -_box_dim.x / 2.f) {
                position.x = -_box_dim.x / 2.f;
                velocity.x = 0.0f;
            } else if (
                position.x > _box_dim.x / 2.f) {
                position.x = _box_dim.x / 2.f;
                velocity.x = 0.0f;
            }
        }

        if (_box_dim.z > 0.f) {
            if (position.z < -_box_dim.z / 2.f) {
                position.z = -_box_dim.z / 2.f;
                velocity.z = 0.0f;
            } else if (
                position.z > _box_dim.z / 2.f) {
                position.z = _box_dim.z / 2.f;
                velocity.z = 0.0f;
            }
        }
    }
}

}