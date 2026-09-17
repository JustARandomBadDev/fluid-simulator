#ifndef FLUID_SIMULATOR_SPH_PARAMETERS_HPP
#define FLUID_SIMULATOR_SPH_PARAMETERS_HPP

namespace fluid::simulation {

struct SphParameters {
    float particleMass = 0.12f;
    float smoothingRadius = 0.10f;
    float restDensity = 1000.0f;
    float stiffness = 40.0f;
    float viscosity = 0.03f;
};

}

#endif