#pragma once

#include "common/util.hpp"

namespace a2 {

struct FresnelDielectric {
    float eta;

    AtRGB f_r(float mu) { return AtRGB(fresnel(mu, eta)); }
    AtRGB f_t(float mu) { return AI_RGB_WHITE - f_r(mu); }
    AtRGB F_avg() { return AtRGB(F_avg_dielectric(1.0f / eta)); }
};

struct FresnelConductor {
    AtRGB r;
    AtRGB g;

    AtRGB f_r(float mu) { return olefresnel(r, g, mu); }
    AtRGB f_t(float mu) { return AI_RGB_BLACK; }
    AtRGB F_avg() { return F_avg_ole(r, g); }
};
}
