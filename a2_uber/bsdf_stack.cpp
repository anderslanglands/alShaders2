#include "bsdf_stack.hpp"
#include "util.hpp"
#include <cassert>
#include <iostream>
#include <new>

AI_BSDF_EXPORT_METHODS(A2BsdfStackMtd);
std::ostream& operator<<(std::ostream& os, AtVector v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

namespace a2 {

BsdfStack::BsdfStack() : _num_bsdfs(0) {
    memset(_layer_transmission, 0, sizeof(AtRGB) * C_max_bsdfs);
}

AtBSDF* BsdfStack::create(AtShaderGlobals* sg) {
    AtBSDF* bsdf = AiBSDF(sg, AI_RGB_WHITE, A2BsdfStackMtd, sizeof(BsdfStack));
    BsdfStack* bsdf_stack = BsdfStack::get(bsdf);
    // Don't forget to call new() on the memory or the vtable won't be set up...
    new (bsdf_stack) BsdfStack;

    // for (int i = 0; i < sg->bounces; ++i) {
    // std::cerr << "  ";
    //}
    // std::cerr << "CREATED NEW STACK " << (int)sg->bounces << std::endl;

    bsdf_stack->_sg = sg;

    return bsdf;
}

void BsdfStack::add_bsdf(Bsdf* bsdf) {
    if (_num_bsdfs >= C_max_bsdfs) {
        return;
    }
    _bsdfs[_num_bsdfs++] = bsdf;
}

void BsdfStack::init(const AtShaderGlobals* sg) {
    for (int i = 0; i < _num_bsdfs; ++i) {
        _bsdfs[i]->init(sg);
        _lobe_info[i] = _bsdfs[i]->get_lobes()[0];
    }
}

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
    int index = get_lobe_index(lobe_mask);
    AtBSDFLobeSample tmp_lobes[1];
    _bsdfs[index]->sample(u, wavelength, 0x1, need_pdf, out_wi, out_lobe_index,
                          tmp_lobes, _layer_transmission[index]);
    // for (int i = 0; i < _sg->bounces; ++i) {
    // std::cerr << "  ";
    //}
    // std::cerr << "SAMPLE " << index << " " << out_wi.val << std::endl;
    out_lobes[index] = tmp_lobes[0];
    out_lobe_index = index;
    return 1 << index;
}

AtBSDFLobeMask BsdfStack::eval(const AtVector& wi,
                               const AtBSDFLobeMask lobe_mask,
                               const bool need_pdf,
                               AtBSDFLobeSample out_lobes[]) {
    int index = get_lobe_index(lobe_mask);
    // for (int i = 0; i < _sg->bounces; ++i) {
    // std::cerr << "  ";
    //}
    // std::cerr << "EVAL " << index << " " << wi << std::endl;
    AtBSDFLobeSample tmp_lobes[1];
    AtRGB transmission = AI_RGB_WHITE;
    for (int i = 0; i < index; ++i) {
        assert(is_finite(_layer_transmission[i]) &&
               "layer transmission is not finite");
        assert(is_positive(_layer_transmission[i]) &&
               "layer transmission is not positive");
        transmission *= _layer_transmission[i];
    }
    _bsdfs[index]->eval(wi, lobe_mask, need_pdf, tmp_lobes);
    out_lobes[index] = tmp_lobes[0];
    out_lobes[index].weight *= transmission;
    return 1 << index;
}
const AtBSDFLobeInfo* BsdfStack::get_lobes() { return _lobe_info; }
int BsdfStack::get_num_lobes() { return _num_bsdfs; }
}

static void Init(const AtShaderGlobals* sg, AtBSDF* bsdf) {
    a2::BsdfStack* bsdf_stack =
        reinterpret_cast<a2::BsdfStack*>(AiBSDFGetData(bsdf));
    bsdf_stack->init(sg);
    AiBSDFInitLobes(bsdf, bsdf_stack->get_lobes(), bsdf_stack->get_num_lobes());
}

static AtBSDFLobeMask
Sample(const AtBSDF* bsdf, const AtVector rnd, const float wavelength,
       const AtBSDFLobeMask lobe_mask, const bool need_pdf, AtVectorDv& out_wi,
       int& out_lobe_index, AtBSDFLobeSample out_lobes[]) {
    a2::BsdfStack* bsdf_stack =
        reinterpret_cast<a2::BsdfStack*>(AiBSDFGetData(bsdf));
    AtRGB transmission;
    return bsdf_stack->sample(rnd, wavelength, lobe_mask, need_pdf, out_wi,
                              out_lobe_index, out_lobes, transmission);
}

static AtBSDFLobeMask Eval(const AtBSDF* bsdf, const AtVector& wi,
                           const AtBSDFLobeMask lobe_mask, const bool need_pdf,
                           AtBSDFLobeSample out_lobes[]) {
    a2::BsdfStack* bsdf_stack =
        reinterpret_cast<a2::BsdfStack*>(AiBSDFGetData(bsdf));
    return bsdf_stack->eval(wi, lobe_mask, need_pdf, out_lobes);
}
