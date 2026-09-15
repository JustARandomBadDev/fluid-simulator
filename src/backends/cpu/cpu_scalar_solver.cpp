#include "backends/cpu/cpu_scalar_solver.hpp"
#include "simulation/particle.hpp"

#include <algorithm>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace fluid::simulation {

void CpuScalarSolver::init() {

}

void CpuScalarSolver::step(
    std::span<Particle> particles,
    float dt
) {
    dt = std::min(dt, 0.001f);

    const glm::vec3 gravity{0.0f, -9.81f, 0.0f};
    
    const float h2 = SMOOTHING_RADIUS * SMOOTHING_RADIUS;

    // Density
    for (Particle& pi : particles) {
        pi.density = 0.0f;

        for (const Particle& pj : particles) {
            const glm::vec3 rij = pi.position - pj.position;
            const float r2 = glm::dot(rij, rij);

            if (r2 < h2) {
                const float x = h2 - r2;

                pi.density +=
                    PARTICLE_MASS *
                    POLY_SIX *
                    x * x * x;
            }
        }
    }

    // Pressure
    for (Particle& particle : particles) {
        particle.pressure = std::max(
            STIFFNESS * (particle.density - REST_DENSITY),
            0.0f
        );
    }

    // Accelerations
    std::vector<glm::vec3> accelerations(
        particles.size(),
        glm::vec3{0.0f}
    );

    for (std::size_t i = 0; i < particles.size(); ++i) {
        const Particle& pi = particles[i];

        glm::vec3 pressureAcceleration{0.0f};
        glm::vec3 viscosityAcceleration{0.0f};

        for (std::size_t j = 0; j < particles.size(); ++j) {
            if (i == j) {
                continue;
            }

            const Particle& pj = particles[j];

            const glm::vec3 rij = pi.position - pj.position;
            const float r = glm::length(rij);

            if (r <= 0.0f || r >= SMOOTHING_RADIUS) {
                continue;
            }

            const glm::vec3 direction = rij / r;
            const float distanceToEdge = SMOOTHING_RADIUS - r;

            // Pressure
            const glm::vec3 gradW =
                -SPIKY *
                distanceToEdge *
                distanceToEdge *
                direction;

            pressureAcceleration -=
                PARTICLE_MASS *
                (
                    pi.pressure / (pi.density * pi.density) +
                    pj.pressure / (pj.density * pj.density)
                ) *
                gradW;

            // Viscosity
            const float laplacian =
                SPIKY * distanceToEdge;

            viscosityAcceleration +=
                VISCOSITY *
                PARTICLE_MASS *
                (pj.velocity - pi.velocity) /
                pj.density *
                laplacian;
        }

        accelerations[i] =
            pressureAcceleration +
            viscosityAcceleration +
            gravity;
    }

    // Box collision
    for (std::size_t i = 0; i < particles.size(); ++i) {
        Particle& particle = particles[i];

        particle.velocity += accelerations[i] * dt;
        particle.position += particle.velocity * dt;

        if (particle.position.y < 0.0f) {
            particle.position.y = 0.0f;
            particle.velocity.y = 0.0f;
        }

        if (particle.position.x < -BOX_SIZE / 2.f) {
            particle.position.x = -BOX_SIZE / 2.f;
            particle.velocity.x = 0.0f;
        } else if (
            particle.position.x > BOX_SIZE / 2.f) {
            particle.position.x = BOX_SIZE / 2.f;
            particle.velocity.x = 0.0f;
        }

        if (particle.position.z < -BOX_SIZE / 2.f) {
            particle.position.z = -BOX_SIZE / 2.f;
            particle.velocity.z = 0.0f;
        } else if (
            particle.position.z > BOX_SIZE / 2.f) {
            particle.position.z = BOX_SIZE / 2.f;
            particle.velocity.z = 0.0f;
        }
    }
}

}