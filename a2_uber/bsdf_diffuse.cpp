#include "bsdf_diffuse.hpp"
#include <new>

AI_BSDF_EXPORT_METHODS(A2BsdfDiffuseMtd);

namespace a2 {
static const AtString str_diffuse("diffuse");
AtBSDF* BsdfDiffuse::create(AtShaderGlobals* sg, AtRGB weight, AtVector N,
                            AtVector U, float roughness) {
    AtBSDF* bsdf = AiBSDF(sg, weight, A2BsdfDiffuseMtd, sizeof(BsdfDiffuse));
    BsdfDiffuse* bsdf_diff = BsdfDiffuse::get(bsdf);
    new (bsdf_diff) BsdfDiffuse;
    bsdf_diff->N = N;
    bsdf_diff->U = U;
    bsdf_diff->arnold_bsdf =
        AiOrenNayarBSDF(sg, weight, N, roughness, false, str_diffuse);
    bsdf_diff->arnold_methods = AiBSDFGetMethods(bsdf_diff->arnold_bsdf);
    bsdf_diff->_sg = sg;
    bsdf_diff->_weight = weight;
    return bsdf;
}

void BsdfDiffuse::init(const AtShaderGlobals* sg) {
    arnold_methods->Init(sg, arnold_bsdf);
}
AtBSDFLobeMask BsdfDiffuse::sample(const AtVector u, const float wavelength,
                                   const AtBSDFLobeMask lobe_mask,
                                   const bool need_pdf, AtVectorDv& out_wi,
                                   int& out_lobe_index,
                                   AtBSDFLobeSample out_lobes[],
                                   AtRGB& transmission) {
    auto out_lobe_mask =
        arnold_methods->Sample(arnold_bsdf, u, wavelength, 0x1, need_pdf,
                               out_wi, out_lobe_index, out_lobes);
    if (out_lobe_mask == AI_BSDF_LOBE_MASK_NONE)
        return AI_BSDF_LOBE_MASK_NONE;
    out_lobes[0].weight *= _weight;
    transmission = AI_RGB_BLACK;
    return out_lobe_mask;
}
AtBSDFLobeMask BsdfDiffuse::eval(const AtVector& wi,
                                 const AtBSDFLobeMask lobe_mask,
                                 const bool need_pdf,
                                 AtBSDFLobeSample out_lobes[],
                                 AtRGB& transmission) {
    auto out_lobe_mask =
        arnold_methods->Eval(arnold_bsdf, wi, 0x1, need_pdf, out_lobes);
    if (out_lobe_mask == AI_BSDF_LOBE_MASK_NONE)
        return AI_BSDF_LOBE_MASK_NONE;
    out_lobes[0].weight *= _weight;
    transmission = AI_RGB_BLACK;
    return out_lobe_mask;
}
const AtBSDFLobeInfo* BsdfDiffuse::get_lobes() const {
    return AiBSDFGetLobes(arnold_bsdf);
}
int BsdfDiffuse::get_num_lobes() const { return 1; }
bool BsdfDiffuse::has_interior() const { return false; }
}

static void Init(const AtShaderGlobals* sg, AtBSDF* bsdf) {
    a2::BsdfDiffuse* bsdf_mf =
        reinterpret_cast<a2::BsdfDiffuse*>(AiBSDFGetData(bsdf));
    bsdf_mf->init(sg);
    AiBSDFInitLobes(bsdf, bsdf_mf->get_lobes(), bsdf_mf->get_num_lobes());
}

static AtBSDFLobeMask
Sample(const AtBSDF* bsdf, const AtVector rnd, const float wavelength,
       const AtBSDFLobeMask lobe_mask, const bool need_pdf, AtVectorDv& out_wi,
       int& out_lobe_index, AtBSDFLobeSample out_lobes[]) {
    a2::BsdfDiffuse* bsdf_mf =
        reinterpret_cast<a2::BsdfDiffuse*>(AiBSDFGetData(bsdf));
    AtRGB transmission;
    return bsdf_mf->sample(rnd, wavelength, lobe_mask, need_pdf, out_wi,
                           out_lobe_index, out_lobes, transmission);
}

static AtBSDFLobeMask Eval(const AtBSDF* bsdf, const AtVector& wi,
                           const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                           AtBSDFLobeSample out_lobes[]) {
    a2::BsdfDiffuse* bsdf_mf =
        reinterpret_cast<a2::BsdfDiffuse*>(AiBSDFGetData(bsdf));
    AtRGB transmission;
    return bsdf_mf->eval(wi, lobe_mask, need_pdf, out_lobes, transmission);
}
