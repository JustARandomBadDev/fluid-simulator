#include "backends/cpu/cpu_scalar_solver.hpp"

#include "simulation/particles_data.hpp"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>

namespace fluid::simulation {

void CpuScalarSolver::init() {
    _box_position = _config.box.position;
    _box_dim = _config.box.dimensions;

    _grid.init(
        _box_position,
        _box_dim,
        _params.smoothingRadius,
        _config.particleCapacity
    );

    _accelerations.resize(_config.particleCapacity);
    _inverse_densities.resize(_config.particleCapacity);
    _pressure_terms.resize(_config.particleCapacity);
}

void CpuScalarSolver::step(ParticleData& p_particles, float p_dt) {
    if (p_particles.count == 0) return;

    p_dt = std::min(p_dt, _config.maximumTimeStep);

    _grid.rebuild(p_particles);

    #pragma omp parallel
    {
        computeDensityAndPressure(p_particles);
        computeAccelerations(p_particles);
        integrate(p_particles, p_dt);
    }
}

void CpuScalarSolver::computeDensityAndPressure(ParticleData& p_particles) {
    #pragma omp for schedule(static)

    for (std::size_t i = 0; i < p_particles.count; i++) {
        const float pix = p_particles.position_x[i];
        const float piy = p_particles.position_y[i];
        const float piz = p_particles.position_z[i];

        const glm::ivec3& cell_pos = _grid.getParticleCellPosition(i);

        float density = 0.0f;

        for (int z = -1; z <= 1; z++) {
            for (int y = -1; y <= 1; y++) {
                for (int x = -1; x <= 1; x++) {
                    const glm::ivec3 neighbor_pos =
                        cell_pos + glm::ivec3(x, y, z);

                    if (!_grid.contains(neighbor_pos)) continue;

                    const uint32_t cell_index =
                        _grid.get(glm::uvec3(neighbor_pos));
                    const uint32_t offset = _grid.getOffset(cell_index);
                    const uint32_t count = _grid.getCount(cell_index);
                    const uint32_t end = offset + count;

                    for (uint32_t k = offset; k < end; k++) {
                        const uint32_t j = _grid.getParticleIndex(k);

                        const float rx = pix - p_particles.position_x[j];
                        const float ry = piy - p_particles.position_y[j];
                        const float rz = piz - p_particles.position_z[j];

                        const float r2 = rx * rx + ry * ry + rz * rz;

                        if (r2 >= _constants.h2) continue;

                        const float q = _constants.h2 - r2;

                        density += _constants.density_factor * q * q * q;
                    }
                }
            }
        }

        const float pressure =
            std::max(_params.stiffness * (density - _params.restDensity), 0.0f);
        const float safe_density = std::max(density, 1e-6f);
        const float inverse_density = 1.0f / safe_density;

        _inverse_densities[i] = inverse_density;
        _pressure_terms[i] = pressure * inverse_density * inverse_density;
    }
}

void CpuScalarSolver::computeAccelerations(ParticleData& p_particles) {
    #pragma omp for schedule(static)

    for (std::size_t i = 0; i < p_particles.count; i++) {
        const float pix = p_particles.position_x[i];
        const float piy = p_particles.position_y[i];
        const float piz = p_particles.position_z[i];

        const float vix = p_particles.velocity_x[i];
        const float viy = p_particles.velocity_y[i];
        const float viz = p_particles.velocity_z[i];

        const glm::ivec3& cell_pos = _grid.getParticleCellPosition(i);

        const float pressure_i = _pressure_terms[i];

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
                        cell_pos + glm::ivec3(x, y, z);

                    if (!_grid.contains(neighbor_pos)) continue;

                    const uint32_t cell_index =
                        _grid.get(glm::uvec3(neighbor_pos));
                    const uint32_t offset = _grid.getOffset(cell_index);
                    const uint32_t count = _grid.getCount(cell_index);
                    const uint32_t end = offset + count;

                    for (uint32_t k = offset; k < end; k++) {
                        const uint32_t j = _grid.getParticleIndex(k);

                        if (i == j) continue;

                        const float rx = pix - p_particles.position_x[j];
                        const float ry = piy - p_particles.position_y[j];
                        const float rz = piz - p_particles.position_z[j];

                        const float r2 = rx * rx + ry * ry + rz * rz;

                        if (r2 <= 0.0f || r2 >= _constants.h2) continue;

                        const float r = std::sqrt(r2);
                        const float inverse_r = 1.0f / r;

                        const float distance_to_edge =
                            _params.smoothingRadius - r;
                        const float distance_to_edge2 =
                            distance_to_edge * distance_to_edge;

                        const float pressure_scalar =
                            _constants.pressure_factor *
                            (pressure_i + _pressure_terms[j]) *
                            distance_to_edge2 * inverse_r;

                        pressure_x += rx * pressure_scalar;
                        pressure_y += ry * pressure_scalar;
                        pressure_z += rz * pressure_scalar;

                        const float viscosity_scalar =
                            _constants.viscosity_factor *
                            _inverse_densities[j] * distance_to_edge;

                        viscosity_x += (p_particles.velocity_x[j] - vix) *
                                       viscosity_scalar;
                        viscosity_y += (p_particles.velocity_y[j] - viy) *
                                       viscosity_scalar;
                        viscosity_z += (p_particles.velocity_z[j] - viz) *
                                       viscosity_scalar;
                    }
                }
            }
        }

        _accelerations[i] = {pressure_x + viscosity_x + _config.gravity.x,
            pressure_y + viscosity_y + _config.gravity.y,
            pressure_z + viscosity_z + _config.gravity.z};
    }
}

void CpuScalarSolver::integrate(ParticleData& p_particles, float p_dt) {
    #pragma omp for schedule(static)

    for (std::size_t i = 0; i < p_particles.count; i++) {
        float& position_x = p_particles.position_x[i];
        float& position_y = p_particles.position_y[i];
        float& position_z = p_particles.position_z[i];

        float& velocity_x = p_particles.velocity_x[i];
        float& velocity_y = p_particles.velocity_y[i];
        float& velocity_z = p_particles.velocity_z[i];

        const auto& acceleration = _accelerations[i];

        velocity_x += acceleration.x * p_dt;
        velocity_y += acceleration.y * p_dt;
        velocity_z += acceleration.z * p_dt;

        position_x += velocity_x * p_dt;
        position_y += velocity_y * p_dt;
        position_z += velocity_z * p_dt;

        applyBoxCollision(
            position_x,
            position_y,
            position_z,
            velocity_x,
            velocity_y,
            velocity_z
        );
    }
}

void CpuScalarSolver::applyBoxCollision(
    float& p_position_x,
    float& p_position_y,
    float& p_position_z,
    float& p_velocity_x,
    float& p_velocity_y,
    float& p_velocity_z
) const {
    if (_box_dim.y > 0.0f) {
        if (p_position_y < _box_position.y) {
            p_position_y = _box_position.y;
            p_velocity_y = 0.0f;
        } else if (p_position_y > _box_position.y + _box_dim.y) {
            p_position_y = _box_position.y + _box_dim.y;

            p_velocity_y = 0.0f;
        }
    }

    if (_box_dim.x > 0.0f) {
        if (p_position_x < _box_position.x) {
            p_position_x = _box_position.x;
            p_velocity_x = 0.0f;
        } else if (p_position_x > _box_position.x + _box_dim.x) {
            p_position_x = _box_position.x + _box_dim.x;

            p_velocity_x = 0.0f;
        }
    }

    if (_box_dim.z > 0.0f) {
        if (p_position_z < _box_position.z) {
            p_position_z = _box_position.z;
            p_velocity_z = 0.0f;
        } else if (p_position_z > _box_position.z + _box_dim.z) {
            p_position_z = _box_position.z + _box_dim.z;

            p_velocity_z = 0.0f;
        }
    }
}

} // namespace fluid::simulation
