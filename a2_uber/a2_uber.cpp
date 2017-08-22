#include "bsdf_diffuse.hpp"
#include "bsdf_ggx_multiscatter.hpp"
#include "bsdf_microfacet.hpp"
#include "bsdf_microfacet_refraction.hpp"
#include "bsdf_stack.hpp"
#include "common/util.hpp"
#include <ai.h>
#include <iostream>
#include <spdlog/fmt/ostr.h>

AI_SHADER_NODE_EXPORT_METHODS(A2UberMtd);

class Uber {

public:
    [[ a2::label("Param 1"), 
       a2::help("blah blah blah") 
       a2::page("Diffuse/Advanced"]] 
    float param_1;
};

node_parameters { AiParameterFlt("roughness", 0.0f); }

node_initialize {}

node_update {}

node_finish {}

shader_evaluate {
    auto U = sg->dPdu;
    auto V = sg->dPdv;
    if (AiV3IsSmall(U)) {
        AiV3BuildLocalFrame(U, V, sg->Nf);
    }

    float roughness = AiShaderEvalParamFlt(0);

    auto bsdf_microfacet_wrap = a2::BsdfMicrofacet::create(
        sg, AtRGB(1), sg->Nf, sg->dPdu, 1.0f, 1.5f, roughness, roughness);

    auto bsdf_microfacet_refraction = a2::BsdfMicrofacetRefraction::create(
        sg, AtRGB(1), sg->Nf, sg->dPdu, 1.0f, 1.5f, 0, 0);

    auto bsdf_oren_nayar =
        a2::BsdfDiffuse::create(sg, AtRGB(0.18f), sg->Nf, sg->dPdu, 0.0f);

    auto bsdf_stack = a2::BsdfStack::create(sg);
    bsdf_stack->add_bsdf(bsdf_microfacet_wrap);
    // bsdf_stack->add_bsdf(bsdf_oren_nayar);
    // bsdf_stack->add_bsdf(bsdf_microfacet_refraction);
    // sg->out.CLOSURE() = bsdf_stack->get_arnold_bsdf();
    auto clist = AtClosureList();
    clist.add(bsdf_microfacet_wrap->get_arnold_bsdf());
    auto bsdf_ms = a2::BsdfGGXMultiscatter::create(sg, sg->Nf, roughness);
    // clist.add(AiOrenNayarBSDF(sg, AtRGB(ms_compensation), sg->Nf));
    clist.add(bsdf_ms->get_arnold_bsdf());
    sg->out.CLOSURE() = clist;
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
