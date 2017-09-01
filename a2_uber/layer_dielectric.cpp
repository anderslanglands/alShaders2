#include "layer_dielectric.hpp"
#include "lut_ggx_F.hpp"
#include <new>
#include <spdlog/fmt/ostr.h>

AI_BSDF_EXPORT_METHODS(A2LayerDielectricMtd);

namespace a2 {

auto lut_ggx_F = get_lut_ggx_F();

auto LayerDielectric::create(AtShaderGlobals* sg, AtVector N, AtVector U,
                             float medium_ior, float ior, float rx, float ry)
    -> LayerDielectric* {
    auto bsdf =
        AiBSDF(sg, AtRGB(1), A2LayerDielectricMtd, sizeof(LayerDielectric));
    auto lyr = reinterpret_cast<LayerDielectric*>(AiBSDFGetData(bsdf));
    new (lyr) LayerDielectric;
    auto mr = AiBSDFMinRoughness(sg);
    rx = std::max(mr, rx);
    ry = std::max(mr, ry);
    lyr->_roughness = std::min(rx, ry);
    lyr->_refl = create_microfacet_dielectric(sg, AtRGB(1), N, U, medium_ior,
                                              ior, rx, ry);
    lyr->_refr = BsdfMicrofacetRefraction::create(
        sg, AtRGB(1), N, U, medium_ior, ior, rx, ry, false);
    lyr->_arnold_bsdf = bsdf;
    return lyr;
}

auto LayerDielectric::init(const AtShaderGlobals* sg) -> void {
    _sg = const_cast<AtShaderGlobals*>(sg);
    _omega_o = AiV3Normalize(-sg->Rd);
    _refl->init(sg);
    _refr->init(sg);
    _sampled_lobe = -1;
}

auto LayerDielectric::sample(const AtVector u, const float wavelength,
                             const AtBSDFLobeMask lobe_mask,
                             const bool need_pdf, AtVectorDv& out_wi,
                             int& out_lobe_index, AtBSDFLobeSample out_lobes[],
                             AtRGB& out_k_r, AtRGB& out_k_t) -> AtBSDFLobeMask {
    if (_sg->Rt == AI_RAY_DIFFUSE_REFLECT)
        return AI_BSDF_LOBE_MASK_NONE;

    float eta = 1.0f / 1.5f;
    float mu_o = dot(_sg->N, _omega_o);
    if (mu_o < 0) {
        // eta = 1.5f;
        mu_o = -mu_o;
    }
    float kr_avg = fresnel(mu_o, eta);

    AtRGB k_r, k_t;
    AtBSDFLobeMask out_lobe_mask = AI_BSDF_LOBE_MASK_NONE;

    if (_sg->bounces > 100) {
        if (_sampled_lobe == -1) {
            // nothing sampled yet, choose a lobe
            // float c = 0.5f;
            float c = kr_avg;
            if (u.z < c) {
                _sampled_lobe = 0;
                _p_c = c;
            } else {
                _sampled_lobe = 1;
                _p_c = 1.0f - c;
            }
        }

        if (lobe_mask & (1 << _sampled_lobe)) {

            if (lobe_mask & (1 << 0)) {
                AtBSDFLobeSample tmp_out_lobes[1];
                out_lobe_mask =
                    _refl->sample(u, wavelength, lobe_mask, need_pdf, out_wi,
                                  out_lobe_index, tmp_out_lobes, k_r, k_t);
                out_lobes[0] = tmp_out_lobes[0];
                out_lobe_index = 0;
            } else {
                AtBSDFLobeSample tmp_out_lobes[1];
                out_lobe_mask =
                    _refr->sample(u, wavelength, lobe_mask, need_pdf, out_wi,
                                  out_lobe_index, tmp_out_lobes, k_r, k_t);
                out_lobes[1] = tmp_out_lobes[0];
                out_lobe_index = 1;
            }
            return out_lobe_mask;
        } else {
            return AI_BSDF_LOBE_MASK_NONE;
        }
    } else {

        if (lobe_mask & (1 << 0)) {
            AtBSDFLobeSample tmp_out_lobes[1];
            out_lobe_mask =
                _refl->sample(u, wavelength, lobe_mask, need_pdf, out_wi,
                              out_lobe_index, tmp_out_lobes, k_r, k_t);
            if (out_lobe_mask != AI_BSDF_LOBE_MASK_NONE) {
                out_lobe_mask = (1 << 0);
            }
            auto mu = fabsf(dot(_sg->N, _omega_o));
            k_r = lut_ggx_F->lookup(_roughness, 1.0f - mu, 1.5f);
            a2assert(is_finite(k_r), "k_r was not finite: {}", k_r);
            a2assert(is_positive(k_r), "k_r was not positive: {}", k_r);
            tmp_out_lobes[0].weight *= k_r;
            out_lobes[0] = tmp_out_lobes[0];
            out_lobe_index = 0;
        } else {
            AtBSDFLobeSample tmp_out_lobes[1];
            out_lobe_mask =
                _refr->sample(u, wavelength, lobe_mask, need_pdf, out_wi,
                              out_lobe_index, tmp_out_lobes, k_r, k_t);
            if (out_lobe_mask != AI_BSDF_LOBE_MASK_NONE) {
                out_lobe_mask = (1 << 1);
            }
            auto H = AiV3Normalize(out_wi.val + _omega_o);
            auto mu = fabsf(dot(_sg->N, _omega_o));
            k_t = 1.0f - lut_ggx_F->lookup(_roughness, 1.0f - mu, 1.5f);
            a2assert(is_finite(k_t), "k_t was not finite: {}", k_t);
            a2assert(is_positive(k_t), "k_t was not positive: {}", k_t);
            tmp_out_lobes[0].weight *= k_t;
            out_lobes[1] = tmp_out_lobes[0];
            out_lobe_index = 1;
        }
    }

    return out_lobe_mask;
}
auto LayerDielectric::eval(const AtVector& wi, const AtBSDFLobeMask lobe_mask,
                           const bool need_pdf, AtBSDFLobeSample out_lobes[],
                           AtRGB& out_k_r, AtRGB& out_k_t) -> AtBSDFLobeMask {
    AtRGB k_r, k_t;
#if 0
    if (_sg->Rt == AI_RAY_DIFFUSE_REFLECT)
        return AI_BSDF_LOBE_MASK_NONE;

    AtBSDFLobeMask out_lobe_mask;
    float selection_pdf = .50f;
    if (wi == _omega_s) {
        selection_pdf = 0.5f;
        if (_sampled_lobe == 0) {
            out_lobe_mask =
                _refl->eval(wi, lobe_mask, need_pdf, out_lobes, transmission);
            out_lobes[0].weight /= selection_pdf;
            out_lobes[0].pdf *= selection_pdf;
            return out_lobe_mask;
        } else {
            out_lobe_mask =
                _refr->eval(wi, lobe_mask, need_pdf, out_lobes, transmission);
            out_lobes[0].weight /= selection_pdf;
            out_lobes[0].pdf *= selection_pdf;
            return out_lobe_mask;
        }
    }
    // if the sampled direction is not the one we sampled ourselves earlier,
    // just eval
    out_lobe_mask =
        _refl->eval(wi, lobe_mask, need_pdf, out_lobes, transmission);
    // we can't do *both* reflection and refraction from the same direction
    return out_lobe_mask;
    // out_lobe_mask =
    //_refr->eval(wi, lobe_mask, need_pdf, out_lobes, transmission);
    // if (out_lobe_mask != AI_BSDF_LOBE_MASK_NONE) {
    // out_lobes[0].weight *= 0.5f / selection_pdf;
    // out_lobes[0].pdf /= selection_pdf;
    //}
    // return out_lobe_mask;
#endif
    if (_sg->Rt == AI_RAY_DIFFUSE_REFLECT)
        return AI_BSDF_LOBE_MASK_NONE;

    AtBSDFLobeMask out_lobe_mask = AI_BSDF_LOBE_MASK_NONE;
    AtBSDFLobeSample tmp_out_lobes[1];
    if ((lobe_mask & (1 << 0))) {
        if (_refl->eval(wi, lobe_mask, need_pdf, tmp_out_lobes, k_r, k_t) !=
            AI_BSDF_LOBE_MASK_NONE) {
            auto mu = fabsf(dot(_sg->N, _omega_o));
            k_r = lut_ggx_F->lookup(_roughness, 1.0f - mu, 1.5f);
            a2assert(is_finite(k_r), "k_r was not finite: {}", k_r);
            a2assert(is_positive(k_r), "k_r was not positive: {}", k_r);
            tmp_out_lobes[0].weight *= k_r;
            out_lobes[0] = tmp_out_lobes[0];
            out_lobe_mask |= 1 << 0;
        } else {
            out_lobe_mask &= ~(1 << 0);
        }
    }
    if ((lobe_mask & (1 << 1))) {
        if (_refr->eval(wi, lobe_mask, need_pdf, tmp_out_lobes, k_r, k_t) !=
            AI_BSDF_LOBE_MASK_NONE) {
            auto mu = fabsf(dot(_sg->N, _omega_o));
            k_r = lut_ggx_F->lookup(_roughness, 1.0f - mu, 1.5f);
            a2assert(is_finite(k_r), "k_r was not finite: {}", k_r);
            a2assert(is_positive(k_r), "k_r was not positive: {}", k_r);
            tmp_out_lobes[0].weight *= (1.0f - k_r);
            out_lobes[1] = tmp_out_lobes[0];
            out_lobe_mask |= 1 << 1;
        } else {
            out_lobe_mask &= ~(1 << 1);
        }
    }
    return out_lobe_mask;
}

auto LayerDielectric::get_lobes() const -> const AtBSDFLobeInfo* {
    static const AtBSDFLobeInfo lobe_info[2] = {
        {AI_RAY_SPECULAR_REFLECT, 0, AtString()},
        {AI_RAY_SPECULAR_TRANSMIT, 0, AtString()}};
    return lobe_info;
};

auto LayerDielectric::get_num_lobes() const -> int { return 2; }

auto LayerDielectric::has_interior() const -> bool {
    return _refr->has_interior();
}

auto LayerDielectric::get_interior(const AtShaderGlobals* sg) -> AtClosureList {
    return _refr->get_interior(sg);
}
auto LayerDielectric::get_arnold_bsdf() -> AtBSDF* { return _arnold_bsdf; }
}

bsdf_init {
    auto lyr = reinterpret_cast<a2::LayerDielectric*>(AiBSDFGetData(bsdf));
    lyr->init(sg);
    AiBSDFInitLobes(bsdf, lyr->get_lobes(), lyr->get_num_lobes());
}

bsdf_sample {
    auto lyr = reinterpret_cast<a2::LayerDielectric*>(AiBSDFGetData(bsdf));
    AtRGB k_r, k_t;
    return lyr->sample(rnd, wavelength, lobe_mask, need_pdf, out_wi,
                       out_lobe_index, out_lobes, k_r, k_t);
}

bsdf_eval {
    auto lyr = reinterpret_cast<a2::LayerDielectric*>(AiBSDFGetData(bsdf));
    AtRGB k_r, k_t;
    return lyr->eval(wi, lobe_mask, need_pdf, out_lobes, k_r, k_t);
}

bsdf_interior {
    auto lyr = reinterpret_cast<a2::LayerDielectric*>(AiBSDFGetData(bsdf));
    return lyr->get_interior(sg);
}
