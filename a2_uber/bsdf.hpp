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
                                  AtBSDFLobeSample out_lobes[], AtRGB& kr,
                                  AtRGB& kt) = 0;
    virtual AtBSDFLobeMask eval(const AtVector& wi,
                                const AtBSDFLobeMask lobe_mask,
                                const bool need_pdf,
                                AtBSDFLobeSample out_lobes[], AtRGB& kr,
                                AtRGB& kt) = 0;
    virtual const AtBSDFLobeInfo* get_lobes() const = 0;
    virtual int get_num_lobes() const = 0;
    virtual bool has_interior() const = 0;
    virtual AtClosureList get_interior(const AtShaderGlobals* sg) = 0;
    virtual AtBSDF* get_arnold_bsdf() = 0;
};
}
