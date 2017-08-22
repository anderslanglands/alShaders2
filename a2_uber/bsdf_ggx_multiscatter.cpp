#include "bsdf_ggx_multiscatter.hpp"
#include "common/util.hpp"
#include "lut_ggx_E.hpp"
#include <new>

AI_BSDF_EXPORT_METHODS(A2BsdfGGXMultiscatterMtd);
namespace a2 {

std::unique_ptr<LUT2D<float>> BsdfGGXMultiscatter::_lut_ggx_E(get_lut_ggx_E());

auto BsdfGGXMultiscatter::create(AtShaderGlobals* sg, AtVector N,
                                 float roughness) -> BsdfGGXMultiscatter* {
    AtBSDF* bsdf = AiBSDF(sg, AtRGB(1), A2BsdfGGXMultiscatterMtd,
                          sizeof(BsdfGGXMultiscatter));
    BsdfGGXMultiscatter* bsdf_ms =
        reinterpret_cast<BsdfGGXMultiscatter*>(AiBSDFGetData(bsdf));
    new (bsdf_ms) BsdfGGXMultiscatter();
    bsdf_ms->_N = N;
    bsdf_ms->_arnold_bsdf = bsdf;
    bsdf_ms->_roughness = roughness;
    return bsdf_ms;
}

auto BsdfGGXMultiscatter::init(const AtShaderGlobals* sg) -> void {
    _Ns = (sg->Ngf == sg->Ng) ? sg->Ns : -sg->Ns;
    _Ng = sg->Ngf;
    _omega_o = -sg->Rd;
    AiBSDFInitNormal(_arnold_bsdf, _N, true);
}

auto BsdfGGXMultiscatter::get_lobes() const -> const AtBSDFLobeInfo* {
    static const AtBSDFLobeInfo lobe_info[1] = {
        {AI_RAY_DIFFUSE_REFLECT, 0, AtString()}};
    return lobe_info;
}

auto BsdfGGXMultiscatter::get_num_lobes() const -> int { return 1; }
auto BsdfGGXMultiscatter::sample(const AtVector u, const float wavelength,
                                 const AtBSDFLobeMask lobe_mask,
                                 const bool need_pdf, AtVectorDv& out_wi,
                                 int& out_lobe_index,
                                 AtBSDFLobeSample out_lobes[],
                                 AtRGB& transmission) -> AtBSDFLobeMask {

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

    // since we have perfect importance sampling, the weight (BRDF / pdf) is 1
    // except for the bump shadowing, which is used to avoid artifacts when the
    // shading normal differs significantly from the smooth surface normal
    const AtRGB weight =
        AtRGB(AiBSDFBumpShadow(_Ns, _N, wi) *
              _lut_ggx_E->lookup(_roughness, 1.0f - dot(_N, _omega_o)));

    // pdf for cosine weighted importance sampling
    const float pdf = mu_i * AI_ONEOVERPI;

    // return output direction vectors, we don't compute differentials here
    out_wi = AtVectorDv(wi);

    // specify that we sampled the first (and only) lobe
    out_lobe_index = 0;

    // return weight and pdf
    out_lobes[0] = AtBSDFLobeSample(weight, 0.0f, pdf);

    // indicate that we have valid lobe samples for all the requested lobes,
    // which is just one lobe in this case
    return lobe_mask;
}

auto BsdfGGXMultiscatter::eval(const AtVector& wi,
                               const AtBSDFLobeMask lobe_mask,
                               const bool need_pdf,
                               AtBSDFLobeSample out_lobes[],
                               AtRGB& transmission) -> AtBSDFLobeMask {
    // discard rays below the hemisphere
    const float mu = dot(_N, wi);
    if (mu <= 0.f)
        return AI_BSDF_LOBE_MASK_NONE;

    // return weight and pdf, same as in bsdf_sample
    const float weight = AiBSDFBumpShadow(_Ns, _N, wi);
    const float pdf = mu * AI_ONEOVERPI;
    out_lobes[0] = AtBSDFLobeSample(AtRGB(weight), 0.0f, pdf);

    return lobe_mask;
}

auto BsdfGGXMultiscatter::has_interior() const -> bool { return false; }
}

static void Init(const AtShaderGlobals* sg, AtBSDF* bsdf) {
    a2::BsdfGGXMultiscatter* bsdf_ms =
        reinterpret_cast<a2::BsdfGGXMultiscatter*>(AiBSDFGetData(bsdf));
    bsdf_ms->init(sg);
    AiBSDFInitLobes(bsdf, bsdf_ms->get_lobes(), bsdf_ms->get_num_lobes());
}

static AtBSDFLobeMask
Sample(const AtBSDF* bsdf, const AtVector rnd, const float wavelength,
       const AtBSDFLobeMask lobe_mask, const bool need_pdf, AtVectorDv& out_wi,
       int& out_lobe_index, AtBSDFLobeSample out_lobes[]) {
    a2::BsdfGGXMultiscatter* bsdf_ms =
        reinterpret_cast<a2::BsdfGGXMultiscatter*>(AiBSDFGetData(bsdf));
    AtRGB transmission;
    return bsdf_ms->sample(rnd, wavelength, lobe_mask, need_pdf, out_wi,
                           out_lobe_index, out_lobes, transmission);
}

static AtBSDFLobeMask Eval(const AtBSDF* bsdf, const AtVector& wi,
                           const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                           AtBSDFLobeSample out_lobes[]) {
    a2::BsdfGGXMultiscatter* bsdf_ms =
        reinterpret_cast<a2::BsdfGGXMultiscatter*>(AiBSDFGetData(bsdf));
    AtRGB transmission;
    return bsdf_ms->eval(wi, lobe_mask, need_pdf, out_lobes, transmission);
}
