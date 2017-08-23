#include "bsdf_diffuse.hpp"
#include "bsdf_ggx_multiscatter.hpp"
#include "bsdf_microfacet.hpp"
#include "bsdf_microfacet_refraction.hpp"
#include "bsdf_stack.hpp"
#include "common/util.hpp"
#include <ai.h>
#include <iostream>
#include <spdlog/fmt/ostr.h>

AI_SHADER_NODE_EXPORT_METHODS(A2ConductorMtd);

node_parameters {
    AiParameterFlt("roughness", 0.0f);
    AiParameterBool("compensate", true);
    AiParameterRGB("reflectivity", 0.99, 0.791587, 0.3465);
    AiParameterRGB("edgetint", 0.99, 0.9801, 0.792);
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

    auto reflectivity = AiShaderEvalParamRGB(2);
    auto edgetint = AiShaderEvalParamRGB(3);

    auto anisotropy = AiShaderEvalParamFlt(4);
    float rx = roughness;
    float ry = roughness;
    if (anisotropy != 0.5f) {
        float aniso_t = sqr(fabsf(2.0f * anisotropy - 1.0f));
        rx = (anisotropy >= 0.5f)
                 ? std::max(0.0001f, roughness)
                 : lerp(std::max(0.0001f, roughness), 1.0f, aniso_t);
        ry = (anisotropy <= 0.5f)
                 ? std::max(0.0001f, roughness)
                 : lerp(std::max(0.0001f, roughness), 1.0f, aniso_t);
    }

    auto bsdf_microfacet_reflection = create_microfacet_conductor(
        sg, AtRGB(1), sg->Nf, sg->dPdu, reflectivity, edgetint, rx, ry);

    auto clist = AtClosureList();
    clist.add(bsdf_microfacet_reflection->get_arnold_bsdf());
    if (compensate) {
        auto bsdf_ms = create_ggx_ms_conductor(sg, sg->Nf, std::max(rx, ry),
                                               reflectivity, edgetint);
        clist.add(bsdf_ms->get_arnold_bsdf());
    }
    sg->out.CLOSURE() = clist;
}

node_loader {
    if (i > 0)
        return false;

    node->methods = A2ConductorMtd;
    node->output_type = AI_TYPE_CLOSURE;
    node->name = "a2_conductor";
    node->node_type = AI_NODE_SHADER;
    strcpy(node->version, AI_VERSION);
    return true;
}
