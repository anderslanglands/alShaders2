#pragma once

#include "common/util.hpp"

namespace a2 {

struct FresnelDielectric {
    float eta;

    auto f_r(float mu) const -> AtRGB { return AtRGB(fresnel(mu, eta)); }
    auto f_t(float mu) const -> AtRGB { return AI_RGB_WHITE - f_r(mu); }
    auto F_avg() const -> AtRGB { return AtRGB(F_avg_dielectric(1.0f / eta)); }
};

struct FresnelConductor {
    AtRGB r;
    AtRGB g;

    auto f_r(float mu) const -> AtRGB { return olefresnel(r, g, mu); }
    auto f_t(float mu) const -> AtRGB { return AI_RGB_BLACK; }
    auto F_avg() const -> AtRGB { return F_avg_ole(r, g); }
};
}
