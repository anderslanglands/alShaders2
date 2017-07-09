#pragma once

#include "bsdf_microfacet.hpp"
#include <memory>

namespace a2 {

class BsdfStack : public Bsdf {
    static constexpr int C_max_bsdfs = 8;
    Bsdf* _bsdfs[C_max_bsdfs];
    AtRGB _layer_transmission[C_max_bsdfs];
    AtBSDFLobeInfo _lobe_info[C_max_bsdfs];
    int _num_bsdfs;
    AtShaderGlobals* _sg;

public:
    BsdfStack();
    ~BsdfStack();

    static AtBSDF* create(AtShaderGlobals* sg);

    static BsdfStack* get(const AtBSDF* bsdf) {
        return reinterpret_cast<BsdfStack*>(AiBSDFGetData(bsdf));
    }

    void add_bsdf(Bsdf* bsdf);

    void init(const AtShaderGlobals* sg) override;
    AtBSDFLobeMask sample(const AtVector u, const float wavelength,
                          const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                          AtVectorDv& out_wi, int& out_lobe_index,
                          AtBSDFLobeSample out_lobes[],
                          AtRGB& transmission) override;
    AtBSDFLobeMask eval(const AtVector& wi, const AtBSDFLobeMask lobe_mask,
                        const bool need_pdf,
                        AtBSDFLobeSample out_lobes[]) override;
    const AtBSDFLobeInfo* get_lobes() override;
    int get_num_lobes() override;
};
}
