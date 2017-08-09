#pragma once

#include "bsdf_microfacet.hpp"
#include "common/a2_assert.hpp"
#include <boost/container/static_vector.hpp>
#include <memory>

namespace a2 {

template <class T> using stack_array = boost::container::static_vector<T, 8>;

class BsdfStack : public Bsdf {
    stack_array<Bsdf*> _bsdfs;
    stack_array<AtRGB> _layer_transmission_bsdf;
    stack_array<AtRGB> _layer_transmission_light;
    stack_array<AtBSDFLobeInfo> _lobe_info;
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
                        const bool need_pdf, AtBSDFLobeSample out_lobes[],
                        AtRGB& transmission) override;
    const AtBSDFLobeInfo* get_lobes() const override;
    int get_num_lobes() const override;
    bool has_interior() const override;
    AtClosureList get_interior(const AtShaderGlobals* sg) override;
    const AtBSDF* get_arnold_bsdf() const override {
        a2assert("tried to get arnold bsdf from a stack");
        return nullptr;
    }
};
}
