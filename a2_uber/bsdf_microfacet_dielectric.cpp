#include "bsdf_microfacet.hpp"

AI_BSDF_EXPORT_METHODS(A2BsdfMicrofacetDielectricMtd);
namespace a2 {
static const AtString str_specular("specular");
BsdfMicrofacetDielectric*
create_microfacet_dielectric(AtShaderGlobals* sg, AtRGB weight, AtVector N,
                             AtVector U, float medium_ior, float ior, float rx,
                             float ry) {
    auto bsdf = AiBSDF(sg, weight, A2BsdfMicrofacetDielectricMtd,
                       sizeof(BsdfMicrofacetDielectric));
    auto bsdf_mf =
        reinterpret_cast<a2::BsdfMicrofacetDielectric*>(AiBSDFGetData(bsdf));
    // Don't forget to call new() on the memory or the vtable won't be set up...
    new (bsdf_mf) BsdfMicrofacetDielectric;
    bsdf_mf->N = N;
    bsdf_mf->U = U;
    bsdf_mf->_fresnel = FresnelDielectric{medium_ior / ior};
    bsdf_mf->_omega_o = -sg->Rd;
    bsdf_mf->at_bsdf = bsdf;
    bsdf_mf->arnold_bsdf = AiMicrofacetBSDF(sg, weight, AI_MICROFACET_GGX, N,
                                            &U, 0, rx, ry, 0, str_specular);
    bsdf_mf->_roughness = AtVector2(rx, ry);
    bsdf_mf->arnold_methods = AiBSDFGetMethods(bsdf_mf->arnold_bsdf);
    bsdf_mf->_sg = sg;
    bsdf_mf->_weight = weight;

    return bsdf_mf;
}
}

static void Init(const AtShaderGlobals* sg, AtBSDF* bsdf) {
    auto bsdf_mf =
        reinterpret_cast<a2::BsdfMicrofacetDielectric*>(AiBSDFGetData(bsdf));
    bsdf_mf->init(sg);
    AiBSDFInitLobes(bsdf, bsdf_mf->get_lobes(), bsdf_mf->get_num_lobes());
}

static AtBSDFLobeMask
Sample(const AtBSDF* bsdf, const AtVector rnd, const float wavelength,
       const AtBSDFLobeMask lobe_mask, const bool need_pdf, AtVectorDv& out_wi,
       int& out_lobe_index, AtBSDFLobeSample out_lobes[]) {
    auto bsdf_mf =
        reinterpret_cast<a2::BsdfMicrofacetDielectric*>(AiBSDFGetData(bsdf));
    AtRGB transmission;
    return bsdf_mf->sample(rnd, wavelength, lobe_mask, need_pdf, out_wi,
                           out_lobe_index, out_lobes, transmission);
}

static AtBSDFLobeMask Eval(const AtBSDF* bsdf, const AtVector& wi,
                           const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                           AtBSDFLobeSample out_lobes[]) {
    auto bsdf_mf =
        reinterpret_cast<a2::BsdfMicrofacetDielectric*>(AiBSDFGetData(bsdf));
    AtRGB transmission;
    return bsdf_mf->eval(wi, lobe_mask, need_pdf, out_lobes, transmission);
}
