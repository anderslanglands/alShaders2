#include "bsdf_ggx_multiscatter.hpp"
#include "common/util.hpp"
#include "fresnel.hpp"
#include "lut_ggx_E.hpp"
#include "lut_ggx_E_avg.hpp"
#include <new>

AI_BSDF_EXPORT_METHODS(A2BsdfGGXMultiscatterConductorMtd);
namespace a2 {

auto create_ggx_ms_conductor(AtShaderGlobals* sg, AtVector N, float roughness,
                             AtRGB reflectivity, AtRGB edgetint)
    -> BsdfGGXMultiscatterConductor* {
    AtBSDF* bsdf = AiBSDF(sg, AtRGB(1), A2BsdfGGXMultiscatterConductorMtd,
                          sizeof(BsdfGGXMultiscatterConductor));
    BsdfGGXMultiscatterConductor* bsdf_ms =
        reinterpret_cast<BsdfGGXMultiscatterConductor*>(AiBSDFGetData(bsdf));
    new (bsdf_ms) BsdfGGXMultiscatterConductor();
    bsdf_ms->_N = N;
    bsdf_ms->_arnold_bsdf = bsdf;
    bsdf_ms->_roughness = roughness;
    bsdf_ms->_fresnel = FresnelConductor{reflectivity, edgetint};
    return bsdf_ms;
}
}

static void Init(const AtShaderGlobals* sg, AtBSDF* bsdf) {
    a2::BsdfGGXMultiscatterConductor* bsdf_ms =
        reinterpret_cast<a2::BsdfGGXMultiscatterConductor*>(
            AiBSDFGetData(bsdf));
    bsdf_ms->init(sg);
    AiBSDFInitLobes(bsdf, bsdf_ms->get_lobes(), bsdf_ms->get_num_lobes());
}

static AtBSDFLobeMask
Sample(const AtBSDF* bsdf, const AtVector rnd, const float wavelength,
       const AtBSDFLobeMask lobe_mask, const bool need_pdf, AtVectorDv& out_wi,
       int& out_lobe_index, AtBSDFLobeSample out_lobes[]) {
    a2::BsdfGGXMultiscatterConductor* bsdf_ms =
        reinterpret_cast<a2::BsdfGGXMultiscatterConductor*>(
            AiBSDFGetData(bsdf));
    AtRGB transmission;
    return bsdf_ms->sample(rnd, wavelength, lobe_mask, need_pdf, out_wi,
                           out_lobe_index, out_lobes, transmission);
}

static AtBSDFLobeMask Eval(const AtBSDF* bsdf, const AtVector& wi,
                           const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                           AtBSDFLobeSample out_lobes[]) {
    a2::BsdfGGXMultiscatterConductor* bsdf_ms =
        reinterpret_cast<a2::BsdfGGXMultiscatterConductor*>(
            AiBSDFGetData(bsdf));
    AtRGB transmission;
    return bsdf_ms->eval(wi, lobe_mask, need_pdf, out_lobes, transmission);
}
