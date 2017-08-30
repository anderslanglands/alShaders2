#include "bsdf_diffuse.hpp"
#include "bsdf_ggx_multiscatter.hpp"
#include "bsdf_microfacet.hpp"
#include "bsdf_microfacet_refraction.hpp"
#include "bsdf_stack.hpp"
#include "common/util.hpp"
#include <ai.h>
#include <iostream>
#include <spdlog/fmt/ostr.h>

AI_SHADER_NODE_EXPORT_METHODS(A2DielectricMtd);

node_parameters {
    AiParameterFlt("roughness", 0.0f);
    AiParameterBool("compensate", true);
    AiParameterFlt("ior", 1.5f);
    AiParameterFlt("anisotropy", 0.5f);
}

node_initialize {}

node_update {}

node_finish {}

shader_evaluate {
    using namespace a2;
    auto U = sg->dPdu;
    auto V = sg->dPdv;
    if (AiV3IsSmall(U)) {
        AiV3BuildLocalFrame(U, V, sg->Nf);
    }

    auto roughness = sqr(AiShaderEvalParamFlt(0));
    auto compensate = AiShaderEvalParamBool(1);

    auto ior = AiShaderEvalParamFlt(2);

    auto anisotropy = AiShaderEvalParamFlt(3);
    AtVector2 r = split_roughness(roughness, anisotropy);

    auto bsdf_microfacet_reflection = create_microfacet_dielectric(
        sg, AtRGB(1), sg->Nf, sg->dPdu, 1.0f, ior, r.x, r.y);

    auto clist = AtClosureList();
    clist.add(bsdf_microfacet_reflection->get_arnold_bsdf());
    if (compensate) {
        auto bsdf_ms =
            create_ggx_ms_dielectric(sg, sg->Nf, std::max(r.x, r.y), 1.0f, ior);
        clist.add(bsdf_ms->get_arnold_bsdf());
    }
    sg->out.CLOSURE() = clist;
}

node_loader {
    if (i > 0)
        return false;

    node->methods = A2DielectricMtd;
    node->output_type = AI_TYPE_CLOSURE;
    node->name = "a2_dielectric";
    node->node_type = AI_NODE_SHADER;
    strcpy(node->version, AI_VERSION);
    return true;
}
