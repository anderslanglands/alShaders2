#pragma once

#include "bsdf.hpp"
#include "lut.hpp"
#include <ai.h>

namespace a2 {

class BsdfGGXMultiscatter : public Bsdf {
    AtShaderGlobals* _sg;
    AtBSDF* _arnold_bsdf;
    AtVector _N;
    AtVector _Ns;
    AtVector _Ng;
    AtVector _omega_o;
    float _roughness;

    static std::unique_ptr<LUT2D<float>> _lut_ggx_E;

public:
    static BsdfGGXMultiscatter* create(AtShaderGlobals* sg, AtVector N,
                                       float roughness);

    void init(const AtShaderGlobals* sg) override;
    AtBSDFLobeMask sample(const AtVector u, const float wavelength,
                          const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                          AtVectorDv& out_wi, int& out_lobe_index,
                          AtBSDFLobeSample out_lobes[],
                          AtRGB& transmission) override;
    AtBSDFLobeMask eval(const AtVector& wi, const AtBSDFLobeMask lobe_mask,
                        const bool need_pdf, AtBSDFLobeSample out_lobes[],
                        AtRGB& transmission) override;
    const AtBSDFLobeInfo* get_lobes() const override;
    int get_num_lobes() const override;
    bool has_interior() const override;
    AtBSDF* get_arnold_bsdf() override { return _arnold_bsdf; }
    AtClosureList get_interior(const AtShaderGlobals* sg) override {
        return AtClosureList();
    }
};
}
