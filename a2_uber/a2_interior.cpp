#include "common/util.hpp"
#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(A2InteriorMtd);

node_parameters {
    AiParameterFlt("roughness", 0.0f);
    AiParameterBool("compensate", true);
    AiParameterRGB("albedo", 0.05, 0.2, 0.5);
}

node_initialize {}

node_update {}

node_finish {}

shader_evaluate {
    auto color = AiShaderEvalParamRGB(2);
    auto alpha = a2::color_to_albedo(color);
    float mfp = .1f;
    auto sigma_t = AtRGB(1) / mfp;
    auto sigma_s = alpha / mfp;
    auto sigma_a = sigma_t - sigma_s;
    auto cls_hg =
        AiClosureVolumeHenyeyGreenstein(sg, sigma_a, sigma_s, AtRGB(0), 0.0f);

    auto bsdf_mfr =
        AiMicrofacetRefractionBSDF(sg, AtRGB(1), AI_MICROFACET_GGX, sg->Nf,
                                   nullptr, 1.0f, 0, 0, 0, false, cls_hg);

    sg->out.CLOSURE() = bsdf_mfr;
}

node_loader {
    if (i > 0)
        return false;

    node->methods = A2InteriorMtd;
    node->output_type = AI_TYPE_CLOSURE;
    node->name = "a2_interior";
    node->node_type = AI_NODE_SHADER;
    strcpy(node->version, AI_VERSION);
    return true;
}
