#include "bsdf_stack.hpp"
#include "common/a2_assert.hpp"
#include "util.hpp"
#include <iostream>
#include <new>
#include <spdlog/fmt/fmt.h>

AI_BSDF_EXPORT_METHODS(A2BsdfStackMtd);
std::ostream& operator<<(std::ostream& os, AtRGB v) {
    os << "(" << v.r << ", " << v.g << ", " << v.b << ")";
    return os;
}
std::ostream& operator<<(std::ostream& os, AtVector v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

namespace a2 {

BsdfStack::BsdfStack()
    : _layer_transmission_bsdf{}, _layer_transmission_light{} {}

AtBSDF* BsdfStack::create(AtShaderGlobals* sg) {
    auto bsdf = AiBSDF(sg, AI_RGB_WHITE, A2BsdfStackMtd, sizeof(BsdfStack));
    auto bsdf_stack = BsdfStack::get(bsdf);
    // Don't forget to call new() on the memory or the vtable won't be set up...
    new (bsdf_stack) BsdfStack;

    bsdf_stack->_sg = sg;

    return bsdf;
}

void BsdfStack::add_bsdf(Bsdf* bsdf) {
    // just ignore if we try to add more than the supported number of BSDFs
    if (_bsdfs.size() == _bsdfs.capacity()) {
        return;
    }
    _bsdfs.push_back(bsdf);
}

void BsdfStack::init(const AtShaderGlobals* sg) {
    for (int i = 0; i < _bsdfs.size(); ++i) {
        _bsdfs[i]->init(sg);
        _lobe_info[i] = _bsdfs[i]->get_lobes()[0];
    }
}

// The lobe mask is just a bitfield indicating which lobe we're calculating for.
// This function transforms it into an array index.
int get_lobe_index(AtBSDFLobeMask lobe_mask) {
    int index = 0;
    while (lobe_mask) {
        lobe_mask >>= 1;
        index++;
    }
    return index - 1;
}

AtBSDFLobeMask BsdfStack::sample(const AtVector u, const float wavelength,
                                 const AtBSDFLobeMask lobe_mask,
                                 const bool need_pdf, AtVectorDv& out_wi,
                                 int& out_lobe_index,
                                 AtBSDFLobeSample out_lobes[],
                                 AtRGB& transmission) {
    auto index = get_lobe_index(lobe_mask);
    AtBSDFLobeSample tmp_lobes[1];
    auto mask = _bsdfs[index]->sample(u, wavelength, 0x1, need_pdf, out_wi,
                                      out_lobe_index, tmp_lobes,
                                      _layer_transmission_bsdf[index]);
    if (mask == AI_BSDF_LOBE_MASK_NONE)
        return AI_BSDF_LOBE_MASK_NONE;
    auto stack_transmission = AI_RGB_WHITE;
    for (int i = 0; i < index; ++i) {
        stack_transmission *= _layer_transmission_bsdf[i];
    }
    out_lobes[index] = tmp_lobes[0];
    out_lobes[index].weight *= stack_transmission;
    a2assert(is_finite(out_lobes[index].weight), "weight was not finite: {}",
             out_lobes[index].weight);
    a2assert(is_positive(out_lobes[index].weight), "weight was not finite: {}",
             out_lobes[index].weight);
    out_lobe_index = index;
    transmission = AI_RGB_BLACK;
    return 1 << index;
}

AtBSDFLobeMask BsdfStack::eval(const AtVector& wi,
                               const AtBSDFLobeMask lobe_mask,
                               const bool need_pdf,
                               AtBSDFLobeSample out_lobes[],
                               AtRGB& transmission) {
    int index = get_lobe_index(lobe_mask);
    AtBSDFLobeSample tmp_lobes[1];
    auto mask = _bsdfs[index]->eval(wi, lobe_mask, need_pdf, tmp_lobes,
                                    _layer_transmission_light[index]);
    if (mask == AI_BSDF_LOBE_MASK_NONE)
        return AI_BSDF_LOBE_MASK_NONE;
    auto stack_transmission = AI_RGB_WHITE;
    for (int i = 0; i < index; ++i) {
        stack_transmission *= _layer_transmission_light[i];
    }
    out_lobes[index] = tmp_lobes[0];
    out_lobes[index].weight *= stack_transmission;
    a2assert(is_finite(out_lobes[index].weight), "weight was not finite: {}",
             out_lobes[index].weight);
    a2assert(is_positive(out_lobes[index].weight), "weight was not finite: {}",
             out_lobes[index].weight);
    transmission = AI_RGB_BLACK;
    return 1 << index;
}
const AtBSDFLobeInfo* BsdfStack::get_lobes() const { return _lobe_info.data(); }
int BsdfStack::get_num_lobes() const { return _bsdfs.size(); }
bool BsdfStack::has_interior() const {
    for (const auto bsdf : _bsdfs) {
        if (bsdf->has_interior())
            return true;
    }
    return false;
}

AtClosureList BsdfStack::get_interior(const AtShaderGlobals* sg) {
    for (auto bsdf : _bsdfs) {
        if (bsdf->has_interior()) {
            return bsdf->get_interior(sg);
        }
    }
    return AtClosureList();
}
}

static void Init(const AtShaderGlobals* sg, AtBSDF* bsdf) {
    auto bsdf_stack = reinterpret_cast<a2::BsdfStack*>(AiBSDFGetData(bsdf));
    bsdf_stack->init(sg);
    AiBSDFInitLobes(bsdf, bsdf_stack->get_lobes(), bsdf_stack->get_num_lobes());
}

static AtBSDFLobeMask
Sample(const AtBSDF* bsdf, const AtVector rnd, const float wavelength,
       const AtBSDFLobeMask lobe_mask, const bool need_pdf, AtVectorDv& out_wi,
       int& out_lobe_index, AtBSDFLobeSample out_lobes[]) {
    auto bsdf_stack = reinterpret_cast<a2::BsdfStack*>(AiBSDFGetData(bsdf));
    AtRGB dummy;
    return bsdf_stack->sample(rnd, wavelength, lobe_mask, need_pdf, out_wi,
                              out_lobe_index, out_lobes, dummy);
}

static AtBSDFLobeMask Eval(const AtBSDF* bsdf, const AtVector& wi,
                           const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                           AtBSDFLobeSample out_lobes[]) {
    auto bsdf_stack = reinterpret_cast<a2::BsdfStack*>(AiBSDFGetData(bsdf));
    AtRGB dummy;
    return bsdf_stack->eval(wi, lobe_mask, need_pdf, out_lobes, dummy);
}

bsdf_interior {
    auto bsdf_stack = reinterpret_cast<a2::BsdfStack*>(AiBSDFGetData(bsdf));
    return bsdf_stack->get_interior(sg);
}
