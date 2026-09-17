#include "simulation/sph_constants.hpp"

#include <numbers>

namespace fluid::simulation {

SphConstants makeSphConstants(const SphParameters& p_params) {
    const float h2 =
        p_params.smoothingRadius *
        p_params.smoothingRadius;
    
    const float h6 = h2 * h2 * h2;

    const float h9 = h6 * h2 * p_params.smoothingRadius;

    const float poly6 = 315.0f /
        (64.0f * std::numbers::pi_v<float> * h9);

    const float spiky = 45.0f /
        (std::numbers::pi_v<float> * h6);

    return {
        h2,
        h6,
        h9,
        poly6,
        spiky
    };
}

}
