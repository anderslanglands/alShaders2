#include "bsdf_ggx_multiscatter.hpp"
#include "bsdf_microfacet.hpp"
#include "bsdf_microfacet_refraction.hpp"

namespace a2 {

class LayerDielectric : public Bsdf {
    AtShaderGlobals* _sg;
    BsdfMicrofacetDielectric* _refl;
    BsdfMicrofacetRefraction* _refr;
    AtBSDF* _arnold_bsdf;
    int _sampled_lobe;
    AtVector _omega_o;
    float _p_c;

public:
    static auto create(AtShaderGlobals* sg, AtVector N, AtVector U,
                       float medium_ior, float ior, float rx, float ry)
        -> LayerDielectric*;
    void init(const AtShaderGlobals* sg);
    AtBSDFLobeMask sample(const AtVector u, const float wavelength,
                          const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                          AtVectorDv& out_wi, int& out_lobe_index,
                          AtBSDFLobeSample out_lobes[], AtRGB& k_r, AtRGB& k_t);
    AtBSDFLobeMask eval(const AtVector& wi, const AtBSDFLobeMask lobe_mask,
                        const bool need_pdf, AtBSDFLobeSample out_lobes[],
                        AtRGB& k_r, AtRGB& k_t);
    const AtBSDFLobeInfo* get_lobes() const;
    int get_num_lobes() const;
    bool has_interior() const;
    AtClosureList get_interior(const AtShaderGlobals* sg);
    AtBSDF* get_arnold_bsdf();
};
}
