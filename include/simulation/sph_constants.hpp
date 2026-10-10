#ifndef FLUID_SIMULATOR_SPH_CONSTANTS_HPP
#define FLUID_SIMULATOR_SPH_CONSTANTS_HPP

#include "simulation/sph_parameters.hpp"

namespace fluid::simulation {

struct SphConstants {
    float h2;
    float h6;
    float h9;

    float poly6;
    float spiky;

    float density_factor;

    float pressure_factor;
    float viscosity_factor;
};

SphConstants makeSphConstants(const SphParameters& p_params);

} // namespace fluid::simulation

#endif