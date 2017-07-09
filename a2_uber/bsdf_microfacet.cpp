#include "bsdf_microfacet.hpp"
#include <cassert>
#include <new>

AI_BSDF_EXPORT_METHODS(A2BsdfMicrofacetMtd);

namespace a2 {
AtBSDF* BsdfMicrofacet::create(AtShaderGlobals* sg, AtRGB weight, AtVector N,
                               AtVector U, float medium_ior, float ior,
                               float rx, float ry) {
    AtBSDF* bsdf =
        AiBSDF(sg, weight, A2BsdfMicrofacetMtd, sizeof(BsdfMicrofacet));
    BsdfMicrofacet* bsdf_mf = BsdfMicrofacet::get(bsdf);
    // Don't forget to call new() on the memory or the vtable won't be set up...
    new (bsdf_mf) BsdfMicrofacet;
    bsdf_mf->N = N;
    bsdf_mf->U = U;
    const float eta = medium_ior / ior;
    bsdf_mf->_eta = eta;
    bsdf_mf->_krn = ((eta - 1) * (eta - 1)) / ((eta + 1) * (eta + 1));
    bsdf_mf->_omega_o = -sg->Rd;
    bsdf_mf->arnold_bsdf = AiMicrofacetBSDF(sg, weight, AI_MICROFACET_GGX, N,
                                            &U, bsdf_mf->_eta, rx, ry);
    bsdf_mf->_roughness = AtVector2(rx, ry);
    bsdf_mf->arnold_methods = AiBSDFGetMethods(bsdf_mf->arnold_bsdf);
    bsdf_mf->_sg = sg;
    bsdf_mf->_weight = weight;
    return bsdf;
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
    auto out_lobe_mask =
        arnold_methods->Sample(arnold_bsdf, u, wavelength, 0x1, need_pdf,
                               out_wi, out_lobe_index, out_lobes);
    float kr = AiSchlickFresnel(N, -_omega_o, _krn);
    assert(AiIsFinite(kr) && "kr is not finite");
    assert(kr > 0 && "kr is not positive");
    transmission = AtRGB(1 - kr);
    out_lobes[0].weight *= _weight;
    return out_lobe_mask;
}

AtBSDFLobeMask BsdfMicrofacet::eval(const AtVector& wi,
                                    const AtBSDFLobeMask lobe_mask,
                                    const bool need_pdf,
                                    AtBSDFLobeSample out_lobes[]) {
    auto out_lobe_mask =
        arnold_methods->Eval(arnold_bsdf, wi, 0x1, need_pdf, out_lobes);
    out_lobes[0].weight *= _weight;
    return out_lobe_mask;
}

const AtBSDFLobeInfo* BsdfMicrofacet::get_lobes() {
    return AiBSDFGetLobes(arnold_bsdf);
}

int BsdfMicrofacet::get_num_lobes() { return 1; }

} // namespace a2

static void Init(const AtShaderGlobals* sg, AtBSDF* bsdf) {
    a2::BsdfMicrofacet* bsdf_mf =
        reinterpret_cast<a2::BsdfMicrofacet*>(AiBSDFGetData(bsdf));
    bsdf_mf->init(sg);
    AiBSDFInitLobes(bsdf, bsdf_mf->get_lobes(), bsdf_mf->get_num_lobes());
}

static AtBSDFLobeMask
Sample(const AtBSDF* bsdf, const AtVector rnd, const float wavelength,
       const AtBSDFLobeMask lobe_mask, const bool need_pdf, AtVectorDv& out_wi,
       int& out_lobe_index, AtBSDFLobeSample out_lobes[]) {
    a2::BsdfMicrofacet* bsdf_mf =
        reinterpret_cast<a2::BsdfMicrofacet*>(AiBSDFGetData(bsdf));
    AtRGB transmission;
    return bsdf_mf->sample(rnd, wavelength, lobe_mask, need_pdf, out_wi,
                           out_lobe_index, out_lobes, transmission);
}

static AtBSDFLobeMask Eval(const AtBSDF* bsdf, const AtVector& wi,
                           const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                           AtBSDFLobeSample out_lobes[]) {
    a2::BsdfMicrofacet* bsdf_mf =
        reinterpret_cast<a2::BsdfMicrofacet*>(AiBSDFGetData(bsdf));
    return bsdf_mf->eval(wi, lobe_mask, need_pdf, out_lobes);
}
