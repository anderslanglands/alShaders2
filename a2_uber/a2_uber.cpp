#include "bsdf_diffuse.hpp"
#include "bsdf_microfacet.hpp"
#include "bsdf_stack.hpp"
#include <ai.h>
#include <iostream>

AI_SHADER_NODE_EXPORT_METHODS(A2UberMtd);

class Uber {

public:
    [[ a2::label("Param 1"), 
       a2::help("blah blah blah") 
       a2::page("Diffuse/Advanced"]] 
    float param_1;
};

node_parameters {}

node_initialize {}

node_update {}

node_finish {}

shader_evaluate {
    AtClosureList closures;

    AtVector U = sg->dPdu;
    AtVector V = sg->dPdv;
    if (AiV3IsSmall(U)) {
        AiV3BuildLocalFrame(U, V, sg->Nf);
    }

    AtBSDF* bsdf_microfacet_wrap = a2::BsdfMicrofacet::create(
        sg, AtRGB(1), sg->Nf, sg->dPdu, 1.0f, 1.5f, 0, 0);

    AtBSDF* bsdf_oren_nayar =
        a2::BsdfDiffuse::create(sg, AtRGB(0.18f), sg->Nf, sg->dPdu, 0.0f);

    AtBSDF* bsdf_stack_ai = a2::BsdfStack::create(sg);
    a2::BsdfStack* bsdf_stack = a2::BsdfStack::get(bsdf_stack_ai);
    bsdf_stack->add_bsdf(a2::BsdfMicrofacet::get(bsdf_microfacet_wrap));
    bsdf_stack->add_bsdf(a2::BsdfDiffuse::get(bsdf_oren_nayar));
    sg->out.CLOSURE() = bsdf_stack_ai;

    // closures.add(bsdf_microfacet_wrap);
    // closures.add(bsdf_oren_nayar);
    // sg->out.CLOSURE() = closures;
}

node_loader {
    if (i > 0)
        return false;

    node->methods = A2UberMtd;
    node->output_type = AI_TYPE_CLOSURE;
    node->name = "a2_uber";
    node->node_type = AI_NODE_SHADER;
    strcpy(node->version, AI_VERSION);
    return true;
}
