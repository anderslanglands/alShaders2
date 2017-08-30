#include "bsdf_hapke.hpp"
#include "common/util.hpp"
#include <new>

AI_BSDF_EXPORT_METHODS(A2BsdfHapkeMtd);
namespace a2 {

auto BsdfHapke::create(const AtShaderGlobals* sg, AtRGB albedo, AtVector N)
    -> BsdfHapke* {
    AtBSDF* bsdf = AiBSDF(sg, AtRGB(1), A2BsdfHapkeMtd, sizeof(BsdfHapke));
    BsdfHapke* bsdf_ms = reinterpret_cast<BsdfHapke*>(AiBSDFGetData(bsdf));
    new (bsdf_ms) BsdfHapke();
    bsdf_ms->_N = N;
    bsdf_ms->_arnold_bsdf = bsdf;
    bsdf_ms->_albedo = albedo;
    return bsdf_ms;
}
auto BsdfHapke::init(const AtShaderGlobals* sg) -> void {
    _Ns = (sg->Ngf == sg->Ng) ? sg->Ns : -sg->Ns;
    _Ng = sg->Ngf;
    _omega_o = -sg->Rd;
    AiBSDFInitNormal(_arnold_bsdf, _N, true);
}

auto BsdfHapke::get_lobes() const -> const AtBSDFLobeInfo* {
    static const AtBSDFLobeInfo lobe_info[1] = {
        {AI_RAY_DIFFUSE_REFLECT, 0, AtString()}};
    return lobe_info;
}

auto BsdfHapke::get_num_lobes() const -> int { return 1; }

float H(float u, float alpha) {
    float y = sqrtf(1 - alpha);
    float n = (1 - y) / (1 + y);

    float d =
        1 - u * (1 - y) * ((n * (-u) - (n / 2) + 1) * logf((u + 1) / u) + n);
    return 1 / d;
}

AtRGB H(float u, AtRGB alpha) {
    return AtRGB(H(u, alpha.r), H(u, alpha.g), H(u, alpha.b));
}

auto BsdfHapke::sample(const AtVector u, const float wavelength,
                       const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                       AtVectorDv& out_wi, int& out_lobe_index,
                       AtBSDFLobeSample out_lobes[], AtRGB& k_r, AtRGB& k_t)
    -> AtBSDFLobeMask {

    // sample cosine weighted incoming light direction
    AtVector U, V;
    AiV3BuildLocalFrame(U, V, _N);
    float sin_theta = sqrtf(u.x);
    float phi = 2 * AI_PI * u.y;
    float mu_i = sqrtf(1 - u.x);
    float mu_o = dot(_N, _omega_o);
    AtVector wi =
        sin_theta * cosf(phi) * U + sin_theta * sinf(phi) * V + mu_i * _N;

    k_t = AI_RGB_BLACK;

    // discard rays below the hemisphere
    if (!(dot(wi, _Ng) > 0))
        return AI_BSDF_LOBE_MASK_NONE;

    // since we have perfect importance sampling, the weight (BRDF / pdf) is
    // 1
    // except for the bump shadowing, which is used to avoid artifacts when
    // the
    // shading normal differs significantly from the smooth surface normal
    // const AtRGB weight = _albedo * AiBSDFBumpShadow(_Ns, _N, wi);
    const AtRGB weight = (_albedo * 0.25f * AI_ONEOVERPI) *
                         (H(mu_i, _albedo) * H(mu_o, _albedo)) / (mu_i + mu_o);

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

auto BsdfHapke::eval(const AtVector& wi, const AtBSDFLobeMask lobe_mask,
                     const bool need_pdf, AtBSDFLobeSample out_lobes[],
                     AtRGB& k_r, AtRGB& k_t) -> AtBSDFLobeMask {
    // discard rays below the hemisphere
    const float mu_i = dot(_N, wi);
    const float mu_o = dot(_N, _omega_o);
    k_t = AI_RGB_BLACK;
    if (mu_i <= 0.f)
        return AI_BSDF_LOBE_MASK_NONE;

    // return weight and pdf, same as in bsdf_sample
    // const AtRGB weight = _albedo * AiBSDFBumpShadow(_Ns, _N, wi);
    const AtRGB weight = (_albedo * 0.25f * AI_ONEOVERPI) *
                         (H(mu_i, _albedo) * H(mu_o, _albedo)) / (mu_i + mu_o);
    const float pdf = mu_i * AI_ONEOVERPI;
    out_lobes[0] = AtBSDFLobeSample(weight, 0.0f, pdf);

    return lobe_mask;
}

auto BsdfHapke::has_interior() const -> bool { return false; }

auto BsdfHapke::get_interior(const AtShaderGlobals* sg) -> AtClosureList {
    return AtClosureList();
}

auto BsdfHapke::get_arnold_bsdf() -> AtBSDF* { return _arnold_bsdf; }
}

static void Init(const AtShaderGlobals* sg, AtBSDF* bsdf) {
    a2::BsdfHapke* bsdf_ms =
        reinterpret_cast<a2::BsdfHapke*>(AiBSDFGetData(bsdf));
    bsdf_ms->init(sg);
    AiBSDFInitLobes(bsdf, bsdf_ms->get_lobes(), bsdf_ms->get_num_lobes());
}

static AtBSDFLobeMask
Sample(const AtBSDF* bsdf, const AtVector rnd, const float wavelength,
       const AtBSDFLobeMask lobe_mask, const bool need_pdf, AtVectorDv& out_wi,
       int& out_lobe_index, AtBSDFLobeSample out_lobes[]) {
    a2::BsdfHapke* bsdf_ms =
        reinterpret_cast<a2::BsdfHapke*>(AiBSDFGetData(bsdf));
    AtRGB k_r, k_t;
    return bsdf_ms->sample(rnd, wavelength, lobe_mask, need_pdf, out_wi,
                           out_lobe_index, out_lobes, k_r, k_t);
}

static AtBSDFLobeMask Eval(const AtBSDF* bsdf, const AtVector& wi,
                           const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                           AtBSDFLobeSample out_lobes[]) {
    a2::BsdfHapke* bsdf_ms =
        reinterpret_cast<a2::BsdfHapke*>(AiBSDFGetData(bsdf));
    AtRGB k_r, k_t;
    return bsdf_ms->eval(wi, lobe_mask, need_pdf, out_lobes, k_r, k_t);
}
