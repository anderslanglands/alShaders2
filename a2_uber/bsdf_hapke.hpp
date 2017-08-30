#pragma once

#include "bsdf.hpp"
#include "common/util.hpp"
#include <ai.h>

namespace a2 {

class BsdfHapke : public Bsdf {
    AtShaderGlobals* _sg;
    AtBSDF* _arnold_bsdf;
    AtVector _N;
    AtVector _Ns;
    AtVector _Ng;
    AtVector _omega_o;
    AtRGB _albedo;

public:
    static BsdfHapke* create(const AtShaderGlobals* sg, AtRGB albedo,
                             AtVector N);

    auto init(const AtShaderGlobals* sg) -> void override;
    auto get_lobes() const -> const AtBSDFLobeInfo* override;

    auto get_num_lobes() const -> int override;

    auto sample(const AtVector u, const float wavelength,
                const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                AtVectorDv& out_wi, int& out_lobe_index,
                AtBSDFLobeSample out_lobes[], AtRGB& kr, AtRGB& kt)
        -> AtBSDFLobeMask override;
    auto eval(const AtVector& wi, const AtBSDFLobeMask lobe_mask,
              const bool need_pdf, AtBSDFLobeSample out_lobes[], AtRGB& kr,
              AtRGB& kt) -> AtBSDFLobeMask override;
    auto has_interior() const -> bool override;

    auto get_interior(const AtShaderGlobals* sg) -> AtClosureList override;
    auto get_arnold_bsdf() -> AtBSDF* override;
};
}
