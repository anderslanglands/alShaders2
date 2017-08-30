#pragma once

#include "bsdf.hpp"
#include "fresnel.hpp"
#include "lut.hpp"
#include <ai.h>

namespace a2 {

class BsdfGGXMultiscatterBase : public Bsdf {
protected:
    static std::unique_ptr<LUT2D<float>> _lut_ggx_E;
    static std::unique_ptr<LUT1D<float>> _lut_ggx_E_avg;
};

template <typename FresnelT>
class BsdfGGXMultiscatter : public BsdfGGXMultiscatterBase {
    AtShaderGlobals* _sg;
    AtBSDF* _arnold_bsdf;
    AtVector _N;
    AtVector _Ns;
    AtVector _Ng;
    AtVector _omega_o;
    FresnelT _fresnel;
    float _roughness;

public:
    auto init(const AtShaderGlobals* sg) -> void override {
        _Ns = (sg->Ngf == sg->Ng) ? sg->Ns : -sg->Ns;
        _Ng = sg->Ngf;
        _omega_o = -sg->Rd;
        AiBSDFInitNormal(_arnold_bsdf, _N, true);
    }

    auto get_lobes() const -> const AtBSDFLobeInfo* override {
        static const AtBSDFLobeInfo lobe_info[1] = {
            {AI_RAY_DIFFUSE_REFLECT, 0, AtString()}};
        return lobe_info;
    }

    auto get_num_lobes() const -> int override { return 1; }

    auto sample(const AtVector u, const float wavelength,
                const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                AtVectorDv& out_wi, int& out_lobe_index,
                AtBSDFLobeSample out_lobes[], AtRGB& k_r, AtRGB& k_t)
        -> AtBSDFLobeMask override {

        if (_roughness == 0)
            return AI_BSDF_LOBE_MASK_NONE;

        // sample cosine weighted incoming light direction
        AtVector U, V;
        AiV3BuildLocalFrame(U, V, _N);
        float sin_theta = sqrtf(u.x);
        float phi = 2 * AI_PI * u.y;
        float mu_i = sqrtf(1 - u.x);
        AtVector wi =
            sin_theta * cosf(phi) * U + sin_theta * sinf(phi) * V + mu_i * _N;

        // discard rays below the hemisphere
        if (!(dot(wi, _Ng) > 0))
            return AI_BSDF_LOBE_MASK_NONE;

        const float mu_o = dot(_N, _omega_o);
        const float E_mu_o = _lut_ggx_E->lookup(_roughness, 1.0f - mu_o);

        // This will handle bidir correctly but we run into numerical
        // instability
        // issues. For now, just ignore it. I guarantee I will spend an hour
        // debugging this at some point.
        const float E_mu_i = _lut_ggx_E->lookup(_roughness, 1.0f - mu_i);
        const float E_avg = _lut_ggx_E_avg->lookup(_roughness);
        AtRGB f_ms = (AtRGB(E_mu_i) * AtRGB(E_mu_o)) / (1.0f - E_avg);
        AtRGB F_avg = _fresnel.F_avg();
        k_r = F_avg * (AI_RGB_WHITE - E_avg) / (AI_RGB_WHITE - F_avg * E_avg);
        k_t = AI_RGB_WHITE - k_r;
        // since we have perfect importance sampling, the weight (BRDF / pdf) is
        // 1
        // except for the bump shadowing, which is used to avoid artifacts when
        // the
        // shading normal differs significantly from the smooth surface normal
        const AtRGB weight = AiBSDFBumpShadow(_Ns, _N, wi) * f_ms * k_r;

        // pdf for cosine weighted importance sampling
        const float pdf = mu_i * AI_ONEOVERPI;

        // return output direction vectors, we don't compute differentials here
        out_wi = AtVectorDv(wi);

        // specify that we sampled the first (and only) lobe
        out_lobe_index = 0;

        // return weight and pdf
        out_lobes[0] = AtBSDFLobeSample(AtRGB(weight), 0.0f, pdf);

        // indicate that we have valid lobe samples for all the requested lobes,
        // which is just one lobe in this case
        return lobe_mask;
    }

    auto eval(const AtVector& wi, const AtBSDFLobeMask lobe_mask,
              const bool need_pdf, AtBSDFLobeSample out_lobes[], AtRGB& k_r,
              AtRGB& k_t) -> AtBSDFLobeMask override {
        if (_roughness == 0)
            return AI_BSDF_LOBE_MASK_NONE;
        // discard rays below the hemisphere
        const float mu_i = dot(_N, wi);
        if (mu_i <= 0.f)
            return AI_BSDF_LOBE_MASK_NONE;

        const float mu_o = dot(_N, _omega_o);
        const float E_mu_o = _lut_ggx_E->lookup(_roughness, 1.0f - mu_o);
        // This should handle bidir, but it doesn't give the same result as just
        // doing the out direction
        const float E_mu_i = _lut_ggx_E->lookup(_roughness, 1.0f - mu_i);
        const float E_avg = _lut_ggx_E_avg->lookup(_roughness);
        AtRGB f_ms = (AtRGB(E_mu_i) * AtRGB(E_mu_o)) / (1.0f - E_avg);
        AtRGB F_avg = _fresnel.F_avg();
        k_r = F_avg * (AI_RGB_WHITE - E_avg) / (AI_RGB_WHITE - F_avg * E_avg);
        k_t = AI_RGB_WHITE - k_r;

        // return weight and pdf, same as in bsdf_sample
        const AtRGB weight = AiBSDFBumpShadow(_Ns, _N, wi) * f_ms * k_r;
        const float pdf = mu_i * AI_ONEOVERPI;
        out_lobes[0] = AtBSDFLobeSample(weight, 0.0f, pdf);

        return lobe_mask;
    }

    auto has_interior() const -> bool override { return false; }

    auto get_interior(const AtShaderGlobals* sg) -> AtClosureList override {
        return AtClosureList();
    }

    auto get_arnold_bsdf() -> AtBSDF* override { return _arnold_bsdf; }

    friend BsdfGGXMultiscatter<FresnelDielectric>*
    create_ggx_ms_dielectric(AtShaderGlobals* sg, AtVector N, float roughness,
                             float medium_ior, float ior);
    friend BsdfGGXMultiscatter<FresnelConductor>*
    create_ggx_ms_conductor(AtShaderGlobals* sg, AtVector N, float roughness,
                            AtRGB reflectivity, AtRGB edgetint);
};

using BsdfGGXMultiscatterDielectric = BsdfGGXMultiscatter<FresnelDielectric>;
using BsdfGGXMultiscatterConductor = BsdfGGXMultiscatter<FresnelConductor>;

BsdfGGXMultiscatterDielectric*
create_ggx_ms_dielectric(AtShaderGlobals* sg, AtVector N, float roughness,
                         float medium_ior, float ior);
BsdfGGXMultiscatterConductor*
create_ggx_ms_conductor(AtShaderGlobals* sg, AtVector N, float roughness,
                        AtRGB reflectivity, AtRGB edgetint);
}
