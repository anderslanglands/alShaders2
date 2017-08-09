#include "bsdf_microfacet_refraction.hpp"
#include "common/a2_assert.hpp"
#include "util.hpp"
#include <new>
#include <spdlog/fmt/fmt.h>

AI_BSDF_EXPORT_METHODS(A2BsdfMicrofacetRefractionMtd);
#include <spdlog/fmt/fmt.h>

namespace a2 {
static const AtString str_refraction("refraction");
AtBSDF* BsdfMicrofacetRefraction::create(AtShaderGlobals* sg, AtRGB weight,
                                         AtVector N, AtVector U,
                                         float medium_ior, float ior, float rx,
                                         float ry) {
    auto bsdf = AiBSDF(sg, weight, A2BsdfMicrofacetRefractionMtd,
                       sizeof(BsdfMicrofacetRefraction));
    auto bsdf_mf = BsdfMicrofacetRefraction::get(bsdf);
    // Don't forget to call new() on the memory or the vtable won't be set up...
    new (bsdf_mf) BsdfMicrofacetRefraction;
    bsdf_mf->N = N;
    bsdf_mf->U = U;
    bsdf_mf->_omega_o = -sg->Rd;
    auto eta = ior / medium_ior;
    bsdf_mf->_eta = eta;
    auto sigma_s = AtRGB(1.09f, 1.59f, 1.79f);
    auto sigma_a = AtRGB(0.013f, 0.070f, 0.145f);
    auto hg = AiClosureVolumeHenyeyGreenstein(sg, sigma_a * 10, sigma_s * 10,
                                              AtRGB(0), 0);
    bsdf_mf->arnold_bsdf = AiMicrofacetRefractionBSDF(
        sg, weight, AI_MICROFACET_GGX, N, &U, eta, rx, ry, 0.0f, false, hg,
        AI_BSDF_LOBE_EXIT_BACKGROUND, str_refraction);
    bsdf_mf->_roughness = AtVector2(rx, ry);
    bsdf_mf->arnold_methods = AiBSDFGetMethods(bsdf_mf->arnold_bsdf);
    bsdf_mf->_sg = sg;
    bsdf_mf->_weight = weight;
    return bsdf;
}

void BsdfMicrofacetRefraction::init(const AtShaderGlobals* sg) {
    arnold_methods->Init(sg, arnold_bsdf);
}

AtBSDFLobeMask BsdfMicrofacetRefraction::sample(
    const AtVector u, const float wavelength, const AtBSDFLobeMask lobe_mask,
    const bool need_pdf, AtVectorDv& out_wi, int& out_lobe_index,
    AtBSDFLobeSample out_lobes[], AtRGB& transmission) {
    auto out_lobe_mask =
        arnold_methods->Sample(arnold_bsdf, u, wavelength, 0x1, need_pdf,
                               out_wi, out_lobe_index, out_lobes);
    if (out_lobe_mask == AI_BSDF_LOBE_MASK_NONE)
        return AI_BSDF_LOBE_MASK_NONE;
    transmission = AtRGB(1);
    a2assert(is_positive(transmission), "transmission is not positive: {}",
             transmission);
    out_lobes[0].weight *= _weight;
    a2assert(is_finite(out_lobes[0].weight), "weight was not finite: {}",
             out_lobes[0].weight);
    a2assert(is_positive(out_lobes[0].weight), "weight was not finite: {}",
             out_lobes[0].weight);
    return out_lobe_mask;
}

AtBSDFLobeMask BsdfMicrofacetRefraction::eval(const AtVector& wi,
                                              const AtBSDFLobeMask lobe_mask,
                                              const bool need_pdf,
                                              AtBSDFLobeSample out_lobes[],
                                              AtRGB& transmission) {
    auto out_lobe_mask =
        arnold_methods->Eval(arnold_bsdf, wi, 0x1, need_pdf, out_lobes);
    if (out_lobe_mask == AI_BSDF_LOBE_MASK_NONE)
        return AI_BSDF_LOBE_MASK_NONE;
    transmission = AtRGB(1);
    out_lobes[0].weight *= _weight;
    a2assert(is_finite(out_lobes[0].weight), "weight was not finite: {}",
             out_lobes[0].weight);
    a2assert(is_positive(out_lobes[0].weight), "weight was not finite: {}",
             out_lobes[0].weight);
    return out_lobe_mask;
}

const AtBSDFLobeInfo* BsdfMicrofacetRefraction::get_lobes() const {
    return AiBSDFGetLobes(arnold_bsdf);
}

int BsdfMicrofacetRefraction::get_num_lobes() const {
    return AiBSDFGetNumLobes(arnold_bsdf);
}

bool BsdfMicrofacetRefraction::has_interior() const { return true; }

AtClosureList
BsdfMicrofacetRefraction::get_interior(const AtShaderGlobals* sg) {
    return arnold_methods->Interior(sg, arnold_bsdf);
}

} // namespace a2

static void Init(const AtShaderGlobals* sg, AtBSDF* bsdf) {
    auto bsdf_mf =
        reinterpret_cast<a2::BsdfMicrofacetRefraction*>(AiBSDFGetData(bsdf));
    bsdf_mf->init(sg);
    AiBSDFInitLobes(bsdf, bsdf_mf->get_lobes(), bsdf_mf->get_num_lobes());
}

static AtBSDFLobeMask
Sample(const AtBSDF* bsdf, const AtVector rnd, const float wavelength,
       const AtBSDFLobeMask lobe_mask, const bool need_pdf, AtVectorDv& out_wi,
       int& out_lobe_index, AtBSDFLobeSample out_lobes[]) {
    auto bsdf_mf =
        reinterpret_cast<a2::BsdfMicrofacetRefraction*>(AiBSDFGetData(bsdf));
    AtRGB transmission;
    return bsdf_mf->sample(rnd, wavelength, lobe_mask, need_pdf, out_wi,
                           out_lobe_index, out_lobes, transmission);
}

static AtBSDFLobeMask Eval(const AtBSDF* bsdf, const AtVector& wi,
                           const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                           AtBSDFLobeSample out_lobes[]) {
    auto bsdf_mf =
        reinterpret_cast<a2::BsdfMicrofacetRefraction*>(AiBSDFGetData(bsdf));
    AtRGB transmission;
    return bsdf_mf->eval(wi, lobe_mask, need_pdf, out_lobes, transmission);
}

bsdf_interior {
    auto bsdf_mf =
        reinterpret_cast<a2::BsdfMicrofacetRefraction*>(AiBSDFGetData(bsdf));
    return bsdf_mf->get_interior(sg);
}
