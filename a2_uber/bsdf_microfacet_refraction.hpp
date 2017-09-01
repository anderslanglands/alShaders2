#pragma once

#include "bsdf.hpp"
#include "util.hpp"
#include <ai.h>

namespace a2 {
class BsdfMicrofacetRefraction : public Bsdf {
    AtVector2 _roughness;
    float _eta;
    AtRGB _weight;
    uint8_t _distribution;
    AtShaderGlobals* _sg;
    AtVector _omega_o;

public:
    const AtBSDFMethods* arnold_methods;
    AtBSDF* arnold_bsdf;
    AtVector N;
    AtVector U;

    static auto create(AtShaderGlobals* sg, AtRGB weight, AtVector N,
                       AtVector U, float medium_ior, float ior, float rx,
                       float ry, bool use_fresnel = false)
        -> BsdfMicrofacetRefraction*;

    static auto get(const AtBSDF* bsdf) -> BsdfMicrofacetRefraction* {
        return reinterpret_cast<BsdfMicrofacetRefraction*>(AiBSDFGetData(bsdf));
    }

    void init(const AtShaderGlobals* sg) override;

    auto sample(const AtVector u, const float wavelength,
                const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                AtVectorDv& out_wi, int& out_lobe_index,
                AtBSDFLobeSample out_lobes[], AtRGB& k_r, AtRGB& k_t)
        -> AtBSDFLobeMask override;

    auto eval(const AtVector& wi, const AtBSDFLobeMask lobe_mask,
              const bool need_pdf, AtBSDFLobeSample out_lobes[], AtRGB& k_r,
              AtRGB& k_t) -> AtBSDFLobeMask override;
    auto get_lobes() const -> const AtBSDFLobeInfo* override;
    auto get_num_lobes() const -> int override;
    auto has_interior() const -> bool override;
    auto get_arnold_bsdf() -> AtBSDF* override { return arnold_bsdf; }
    auto get_interior(const AtShaderGlobals* sg) -> AtClosureList override;
};
} // namespace a2
