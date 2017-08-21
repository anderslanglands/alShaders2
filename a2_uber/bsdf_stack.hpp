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
    AtBSDF* _arnold_bsdf;

public:
    BsdfStack();
    ~BsdfStack();

    static auto create(AtShaderGlobals* sg) -> BsdfStack*;

    static auto get(const AtBSDF* bsdf) -> BsdfStack* {
        return reinterpret_cast<BsdfStack*>(AiBSDFGetData(bsdf));
    }

    void add_bsdf(Bsdf* bsdf);

    void init(const AtShaderGlobals* sg) override;

    auto sample(const AtVector u, const float wavelength,
                const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                AtVectorDv& out_wi, int& out_lobe_index,
                AtBSDFLobeSample out_lobes[], AtRGB& transmission)
        -> AtBSDFLobeMask override;

    auto eval(const AtVector& wi, const AtBSDFLobeMask lobe_mask,
              const bool need_pdf, AtBSDFLobeSample out_lobes[],
              AtRGB& transmission) -> AtBSDFLobeMask override;

    auto get_lobes() const -> const AtBSDFLobeInfo* override;

    auto get_num_lobes() const -> int override;

    auto has_interior() const -> bool override;

    auto get_interior(const AtShaderGlobals* sg) -> AtClosureList override;

    auto get_arnold_bsdf() -> AtBSDF* override { return _arnold_bsdf; }
};
}
