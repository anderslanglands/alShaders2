#pragma once

#include "common/util.hpp"

namespace a2 {

struct FresnelDielectric {
    float eta;

    AtRGB f(float mu) { return AtRGB(fresnel(mu, eta)); }
    AtRGB F_avg() { return AtRGB(F_avg_dielectric(eta)); }
};

struct FresnelConductor {
    AtRGB r;
    AtRGB g;

    AtRGB f(float mu) { return olefresnel(r, g, mu); }
    AtRGB F_avg() { return F_avg_ole(r, g); }
};
}
