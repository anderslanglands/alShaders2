#pragma once

#include "bsdf.hpp"
#include "common/a2_assert.hpp"
#include "common/util.hpp"
#include "fresnel.hpp"
#include "lut.hpp"
#include <ai.h>

namespace a2 {

template <typename FresnelT> class BsdfMicrofacet : public Bsdf {
    AtVector2 _roughness;
    FresnelT _fresnel;
    AtRGB _weight;
    uint8_t _distribution;
    AtShaderGlobals* _sg;
    AtVector _omega_o;

public:
    const AtBSDFMethods* arnold_methods;
    AtBSDF* arnold_bsdf;
    AtBSDF* at_bsdf;
    AtVector N;
    AtVector U;

    void init(const AtShaderGlobals* sg) override {
        arnold_methods->Init(sg, arnold_bsdf);
    }

    auto f_r(const float mu) const -> AtRGB { return _fresnel.f_r(mu); }
    auto f_t(const float mu) const -> AtRGB { return _fresnel.f_t(mu); }

    AtBSDFLobeMask sample(const AtVector u, const float wavelength,
                          const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                          AtVectorDv& out_wi, int& out_lobe_index,
                          AtBSDFLobeSample out_lobes[], AtRGB& k_r,
                          AtRGB& k_t) override {
        k_t = AI_RGB_WHITE;
        auto out_lobe_mask =
            arnold_methods->Sample(arnold_bsdf, u, wavelength, 0x1, need_pdf,
                                   out_wi, out_lobe_index, out_lobes);
        if (out_lobe_mask == AI_BSDF_LOBE_MASK_NONE)
            return AI_BSDF_LOBE_MASK_NONE;
        auto H = AiV3Normalize(out_wi.val + _omega_o);
        auto mu = dot(H, out_wi.val);
        if (mu < 0)
            return AI_BSDF_LOBE_MASK_NONE;
        k_r = _fresnel.f_r(mu);
        a2assert(is_finite(k_r), "k_r is not finite: {}", k_r);
        a2assert(is_positive(k_r), "k_r is not positive: {}", k_r);
        k_t = _fresnel.f_t(mu);
        k_t = AI_RGB_WHITE - k_r;
        a2assert(is_positive(k_t), "transmission is not positive: {}", k_t);
        out_lobes[0].weight *= _weight; // * k_r;
        a2assert(is_finite(out_lobes[0].weight), "weight was not finite: {}",
                 out_lobes[0].weight);
        a2assert(is_positive(out_lobes[0].weight), "weight was not finite: {}",
                 out_lobes[0].weight);
        return out_lobe_mask;
    }

    AtBSDFLobeMask eval(const AtVector& wi, const AtBSDFLobeMask lobe_mask,
                        const bool need_pdf, AtBSDFLobeSample out_lobes[],
                        AtRGB& k_r, AtRGB& k_t) override {
        k_t = AI_RGB_WHITE;
        auto out_lobe_mask =
            arnold_methods->Eval(arnold_bsdf, wi, 0x1, need_pdf, out_lobes);
        if (out_lobe_mask == AI_BSDF_LOBE_MASK_NONE)
            return AI_BSDF_LOBE_MASK_NONE;
        auto H = AiV3Normalize(wi + _omega_o);
        auto mu = dot(H, wi);
        if (mu < 0)
            return AI_BSDF_LOBE_MASK_NONE;
        k_r = _fresnel.f_r(mu);
        a2assert(is_finite(k_r), "kr is not finite: {}", k_r);
        a2assert(is_positive(k_r), "kr is not positive: {}", k_r);
        k_t = _fresnel.f_t(mu);
        k_t = AI_RGB_WHITE - k_r;
        a2assert(is_positive(k_t), "transmission is not positive: {}", k_t);
        out_lobes[0].weight *= _weight; // * kr;
        a2assert(is_finite(out_lobes[0].weight), "weight was not finite: {}",
                 out_lobes[0].weight);
        a2assert(is_positive(out_lobes[0].weight), "weight was not finite: {}",
                 out_lobes[0].weight);
        return out_lobe_mask;
    }

    AtBSDF* get_arnold_bsdf() override { return at_bsdf; }
    const AtBSDFLobeInfo* get_lobes() const override {
        return AiBSDFGetLobes(arnold_bsdf);
    }

    int get_num_lobes() const override { return 1; }

    bool has_interior() const override { return false; }
    AtClosureList get_interior(const AtShaderGlobals* sg) override {
        return AtClosureList();
    }

    friend BsdfMicrofacet<FresnelDielectric>*
    create_microfacet_dielectric(AtShaderGlobals* sg, AtRGB weight, AtVector N,
                                 AtVector U, float medium_ior, float ior,
                                 float rx, float ry);

    friend BsdfMicrofacet<FresnelConductor>*
    create_microfacet_conductor(AtShaderGlobals* sg, AtRGB weight, AtVector N,
                                AtVector U, AtRGB reflectivity, AtRGB edgetint,
                                float rx, float ry);
};

using BsdfMicrofacetDielectric = BsdfMicrofacet<FresnelDielectric>;
using BsdfMicrofacetConductor = BsdfMicrofacet<FresnelConductor>;

BsdfMicrofacetDielectric*
create_microfacet_dielectric(AtShaderGlobals* sg, AtRGB weight, AtVector N,
                             AtVector U, float medium_ior, float ior, float rx,
                             float ry);

BsdfMicrofacetConductor*
create_microfacet_conductor(AtShaderGlobals* sg, AtRGB weight, AtVector N,
                            AtVector U, AtRGB reflectivity, AtRGB edgetint,
                            float rx, float ry);

} // namespace a2
