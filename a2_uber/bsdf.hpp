#pragma once

#include <ai.h>

namespace a2 {

class Bsdf {
public:
    virtual void init(const AtShaderGlobals* sg) = 0;
    virtual AtBSDFLobeMask sample(const AtVector u, const float wavelength,
                                  const AtBSDFLobeMask lobe_mask,
                                  const bool need_pdf, AtVectorDv& out_wi,
                                  int& out_lobe_index,
                                  AtBSDFLobeSample out_lobes[],
                                  AtRGB& transmission) = 0;
    virtual AtBSDFLobeMask eval(const AtVector& wi,
                                const AtBSDFLobeMask lobe_mask,
                                const bool need_pdf,
                                AtBSDFLobeSample out_lobes[],
                                AtRGB& transmission) = 0;
    virtual const AtBSDFLobeInfo* get_lobes() = 0;
    virtual int get_num_lobes() = 0;
};
}
