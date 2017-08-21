#include "bsdf_microfacet.hpp"
#include "common/a2_assert.hpp"
#include <new>

#include "util.hpp"
#include <spdlog/fmt/ostr.h>

AI_BSDF_EXPORT_METHODS(A2BsdfMicrofacetMtd);

namespace a2 {
static const AtString str_specular("specular");
BsdfMicrofacet* BsdfMicrofacet::create(AtShaderGlobals* sg, AtRGB weight,
                                       AtVector N, AtVector U, float medium_ior,
                                       float ior, float rx, float ry) {
    auto bsdf = AiBSDF(sg, weight, A2BsdfMicrofacetMtd, sizeof(BsdfMicrofacet));
    auto bsdf_mf = BsdfMicrofacet::get(bsdf);
    // Don't forget to call new() on the memory or the vtable won't be set up...
    new (bsdf_mf) BsdfMicrofacet;
    bsdf_mf->N = N;
    bsdf_mf->U = U;
    const auto eta = medium_ior / ior;
    bsdf_mf->_eta = eta;
    bsdf_mf->_omega_o = -sg->Rd;
    bsdf_mf->arnold_bsdf = AiMicrofacetBSDF(sg, weight, AI_MICROFACET_GGX, N,
                                            &U, 0, rx, ry, 0, str_specular);
    bsdf_mf->_roughness = AtVector2(rx, ry);
    bsdf_mf->arnold_methods = AiBSDFGetMethods(bsdf_mf->arnold_bsdf);
    bsdf_mf->_sg = sg;
    bsdf_mf->_weight = weight;

    return bsdf_mf;
}

void BsdfMicrofacet::init(const AtShaderGlobals* sg) {
    arnold_methods->Init(sg, arnold_bsdf);
}

AtBSDFLobeMask BsdfMicrofacet::sample(const AtVector u, const float wavelength,
                                      const AtBSDFLobeMask lobe_mask,
                                      const bool need_pdf, AtVectorDv& out_wi,
                                      int& out_lobe_index,
                                      AtBSDFLobeSample out_lobes[],
                                      AtRGB& transmission) {
    transmission = AI_RGB_WHITE;
    auto out_lobe_mask =
        arnold_methods->Sample(arnold_bsdf, u, wavelength, 0x1, need_pdf,
                               out_wi, out_lobe_index, out_lobes);
    if (out_lobe_mask == AI_BSDF_LOBE_MASK_NONE)
        return AI_BSDF_LOBE_MASK_NONE;
    auto H = AiV3Normalize(out_wi.val + _omega_o);
    auto mu = dot(H, out_wi.val);
    if (mu < 0)
        return AI_BSDF_LOBE_MASK_NONE;
    auto kr = fresnel(mu, _eta);
    a2assert(is_finite(kr), "kr is not finite: {}", kr);
    a2assert(is_positive(kr), "kr is not positive: {}", kr);
    transmission = AtRGB(1 - kr);
    a2assert(is_positive(transmission), "transmission is not positive: {}",
             transmission);
    // out_lobes[0].weight *= _weight * kr;
    // out_lobes[0].weight +=
    //_roughness_lut->lookup(_roughness.x, 1.0f - dot(_omega_o, N)) *
    // out_lobes[0].pdf;
    a2assert(is_finite(out_lobes[0].weight), "weight was not finite: {}",
             out_lobes[0].weight);
    a2assert(is_positive(out_lobes[0].weight), "weight was not finite: {}",
             out_lobes[0].weight);
    return out_lobe_mask;
}

AtBSDFLobeMask BsdfMicrofacet::eval(const AtVector& wi,
                                    const AtBSDFLobeMask lobe_mask,
                                    const bool need_pdf,
                                    AtBSDFLobeSample out_lobes[],
                                    AtRGB& transmission) {
    transmission = AI_RGB_WHITE;
    auto out_lobe_mask =
        arnold_methods->Eval(arnold_bsdf, wi, 0x1, need_pdf, out_lobes);
    if (out_lobe_mask == AI_BSDF_LOBE_MASK_NONE)
        return AI_BSDF_LOBE_MASK_NONE;
    auto H = AiV3Normalize(wi + _omega_o);
    auto mu = dot(H, wi);
    if (mu < 0)
        return AI_BSDF_LOBE_MASK_NONE;
    auto kr = fresnel(mu, _eta);
    a2assert(is_finite(kr), "kr is not finite: {}", kr);
    a2assert(is_positive(kr), "kr is not positive: {}", kr);
    transmission = AtRGB(1 - kr);
    a2assert(is_positive(transmission), "transmission is not positive: {}",
             transmission);
    // out_lobes[0].weight *= _weight * kr;
    // out_lobes[0].weight +=
    //_roughness_lut->lookup(_roughness.x, 1.0f - dot(_omega_o, N)) *
    // out_lobes[0].pdf;
    a2assert(is_finite(out_lobes[0].weight), "weight was not finite: {}",
             out_lobes[0].weight);
    a2assert(is_positive(out_lobes[0].weight), "weight was not finite: {}",
             out_lobes[0].weight);
    return out_lobe_mask;
}

const AtBSDFLobeInfo* BsdfMicrofacet::get_lobes() const {
    return AiBSDFGetLobes(arnold_bsdf);
}

int BsdfMicrofacet::get_num_lobes() const { return 1; }

bool BsdfMicrofacet::has_interior() const { return false; }

} // namespace a2

static void Init(const AtShaderGlobals* sg, AtBSDF* bsdf) {
    auto bsdf_mf = reinterpret_cast<a2::BsdfMicrofacet*>(AiBSDFGetData(bsdf));
    bsdf_mf->init(sg);
    AiBSDFInitLobes(bsdf, bsdf_mf->get_lobes(), bsdf_mf->get_num_lobes());
}

static AtBSDFLobeMask
Sample(const AtBSDF* bsdf, const AtVector rnd, const float wavelength,
       const AtBSDFLobeMask lobe_mask, const bool need_pdf, AtVectorDv& out_wi,
       int& out_lobe_index, AtBSDFLobeSample out_lobes[]) {
    auto bsdf_mf = reinterpret_cast<a2::BsdfMicrofacet*>(AiBSDFGetData(bsdf));
    AtRGB transmission;
    return bsdf_mf->sample(rnd, wavelength, lobe_mask, need_pdf, out_wi,
                           out_lobe_index, out_lobes, transmission);
}

static AtBSDFLobeMask Eval(const AtBSDF* bsdf, const AtVector& wi,
                           const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                           AtBSDFLobeSample out_lobes[]) {
    auto bsdf_mf = reinterpret_cast<a2::BsdfMicrofacet*>(AiBSDFGetData(bsdf));
    AtRGB transmission;
    return bsdf_mf->eval(wi, lobe_mask, need_pdf, out_lobes, transmission);
}
