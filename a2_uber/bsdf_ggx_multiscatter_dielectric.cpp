#include "bsdf_ggx_multiscatter.hpp"
#include "common/util.hpp"
#include "fresnel.hpp"
#include "lut_ggx_E.hpp"
#include "lut_ggx_E_avg.hpp"
#include <new>

AI_BSDF_EXPORT_METHODS(A2BsdfGGXMultiscatterDielectricMtd);
namespace a2 {

auto create_ggx_ms_dielectric(AtShaderGlobals* sg, AtVector N, float roughness,
                              float medium_ior, float ior)
    -> BsdfGGXMultiscatterDielectric* {
    AtBSDF* bsdf = AiBSDF(sg, AtRGB(1), A2BsdfGGXMultiscatterDielectricMtd,
                          sizeof(BsdfGGXMultiscatterDielectric));
    BsdfGGXMultiscatterDielectric* bsdf_ms =
        reinterpret_cast<BsdfGGXMultiscatterDielectric*>(AiBSDFGetData(bsdf));
    new (bsdf_ms) BsdfGGXMultiscatterDielectric();
    bsdf_ms->_N = N;
    bsdf_ms->_arnold_bsdf = bsdf;
    bsdf_ms->_roughness = roughness;
    bsdf_ms->_fresnel = FresnelDielectric{medium_ior / ior};
    return bsdf_ms;
}
}

static void Init(const AtShaderGlobals* sg, AtBSDF* bsdf) {
    a2::BsdfGGXMultiscatterDielectric* bsdf_ms =
        reinterpret_cast<a2::BsdfGGXMultiscatterDielectric*>(
            AiBSDFGetData(bsdf));
    bsdf_ms->init(sg);
    AiBSDFInitLobes(bsdf, bsdf_ms->get_lobes(), bsdf_ms->get_num_lobes());
}

static AtBSDFLobeMask
Sample(const AtBSDF* bsdf, const AtVector rnd, const float wavelength,
       const AtBSDFLobeMask lobe_mask, const bool need_pdf, AtVectorDv& out_wi,
       int& out_lobe_index, AtBSDFLobeSample out_lobes[]) {
    a2::BsdfGGXMultiscatterDielectric* bsdf_ms =
        reinterpret_cast<a2::BsdfGGXMultiscatterDielectric*>(
            AiBSDFGetData(bsdf));
    AtRGB k_r, k_t;
    return bsdf_ms->sample(rnd, wavelength, lobe_mask, need_pdf, out_wi,
                           out_lobe_index, out_lobes, k_r, k_t);
}

static AtBSDFLobeMask Eval(const AtBSDF* bsdf, const AtVector& wi,
                           const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                           AtBSDFLobeSample out_lobes[]) {
    a2::BsdfGGXMultiscatterDielectric* bsdf_ms =
        reinterpret_cast<a2::BsdfGGXMultiscatterDielectric*>(
            AiBSDFGetData(bsdf));
    AtRGB k_r, k_t;
    return bsdf_ms->eval(wi, lobe_mask, need_pdf, out_lobes, k_r, k_t);
}
