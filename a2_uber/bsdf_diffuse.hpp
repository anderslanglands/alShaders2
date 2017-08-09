#pragma once

#include "bsdf.hpp"
#include <ai.h>

namespace a2 {

class BsdfDiffuse : public Bsdf {
    AtShaderGlobals* _sg;
    const AtBSDFMethods* arnold_methods;
    AtBSDF* arnold_bsdf;
    AtVector N;
    AtVector U;
    AtRGB _weight;

public:
    static BsdfDiffuse* get(const AtBSDF* bsdf) {
        return reinterpret_cast<BsdfDiffuse*>(AiBSDFGetData(bsdf));
    }

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
    const AtBSDF* get_arnold_bsdf() const override { return arnold_bsdf; }
    AtClosureList get_interior(const AtShaderGlobals* sg) override {
        return AtClosureList();
    }

    static AtBSDF* create(AtShaderGlobals* sg, AtRGB weight, AtVector N,
                          AtVector U, float roughness);
};
}
