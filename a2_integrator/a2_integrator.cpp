#include "a2_uber/lut.hpp"
#include "common/util.hpp"
#include <ai.h>
#include <cstdlib>
#include <vector>

AI_SHADER_NODE_EXPORT_METHODS(A2IntegratorMtd);

node_parameters {}

node_initialize {}

node_update {}

node_finish {}

shader_evaluate {
    auto U = sg->dPdu;
    auto V = sg->dPdv;
    if (AiV3IsSmall(U)) {
        AiV3BuildLocalFrame(U, V, sg->Nf);
    }

    AtRGB display = AI_RGB_RED;
    if (sg->x == 0 && sg->y == 0 && getenv("A2_INTEGRATE") != nullptr) {
        auto sg_copy = *sg;

        /*
        int lut_resolution_i = 32;
        int lut_resolution_j = 32;
        int lut_size = lut_resolution_i * lut_resolution_j;
        auto lut = std::vector<float>(lut_resolution_i * lut_resolution_j);

        for (int j = 0; j < lut_resolution_j; ++j) {
            // invert mu as we don't want to capture mu=0
            auto mu = 1.0f - float(j) / float(lut_resolution_j);
            auto sin_theta = cosf(asinf(mu));
            auto omega_o = AtVector(sin_theta, mu, 0.0f);
            sg_copy.Rd = -omega_o;
            for (int i = 0; i < lut_resolution_i; ++i) {
                float r = float(i) / float(lut_resolution_i - 1);
                auto idx = j * lut_resolution_i + i;
                auto bsdf_mf =
                    AiMicrofacetBSDF(&sg_copy, AI_RGB_WHITE, AI_MICROFACET_GGX,
                                     sg_copy.Nf, &U, 0.0f, r, r);
                auto mtd = AiBSDFGetMethods(bsdf_mf);
                mtd->Init(&sg_copy, bsdf_mf);
                float result = 0.0f;
                int samples = 100;
                int ns = 0;
                for (int x = 0; x < samples; ++x) {
                    for (int y = 0; y < samples; ++y) {
                        float xx = float(x) / float(samples);
                        float yy = float(y) / float(samples);
                        AtVectorDv out_wi;
                        int out_lobe_index;
                        AtBSDFLobeSample lobe_sample;
                        auto lobe_mask = mtd->Sample(
                            bsdf_mf, AtVector(xx, yy, 0), 550, 0x1, true,
                            out_wi, out_lobe_index, &lobe_sample);
                        if (lobe_mask != AI_BSDF_LOBE_MASK_NONE) {
                            result += lobe_sample.weight.r;
                            ns++;
                        }
                    }
                }
                result /= float(samples * samples);
                result = 1.0f - result;
                lut[idx] = result;
            }
        }

        auto lut_out = a2::LUT2D<float>(
            lut, a2::Dimension{lut_resolution_i, 0.0f, 1.0f, true},
            a2::Dimension{lut_resolution_j, 0.0f, 1.0f, false});
            */
        auto fun_ggx_roughness_mu = [&](int i, int j, int lut_resolution_i,
                                        int lut_resolution_j) {

            auto mu = 1.0f - float(j) / float(lut_resolution_j);
            auto sin_theta = cosf(asinf(mu));
            auto omega_o = AtVector(sin_theta, mu, 0.0f);
            sg_copy.Rd = -omega_o;
            float r = float(i) / float(lut_resolution_i - 1);
            auto idx = j * lut_resolution_i + i;
            auto bsdf_mf =
                AiMicrofacetBSDF(&sg_copy, AI_RGB_WHITE, AI_MICROFACET_GGX,
                                 sg_copy.Nf, &U, 0.0f, r, r);
            auto mtd = AiBSDFGetMethods(bsdf_mf);
            mtd->Init(&sg_copy, bsdf_mf);
            float result = 0.0f;
            int samples = 100;
            int ns = 0;
            for (int x = 0; x < samples; ++x) {
                for (int y = 0; y < samples; ++y) {
                    float xx = float(x) / float(samples);
                    float yy = float(y) / float(samples);
                    AtVectorDv out_wi;
                    int out_lobe_index;
                    AtBSDFLobeSample lobe_sample;
                    auto lobe_mask =
                        mtd->Sample(bsdf_mf, AtVector(xx, yy, 0), 550, 0x1,
                                    true, out_wi, out_lobe_index, &lobe_sample);
                    if (lobe_mask != AI_BSDF_LOBE_MASK_NONE) {
                        result += lobe_sample.weight.r;
                        ns++;
                    }
                }
            }
            result /= float(samples * samples);
            result = 1.0f - result;
            return result;
        };

        auto lut_out = a2::LUT2D<float>(fun_ggx_roughness_mu,
                                        a2::Dimension{32, 0.0f, 1.0f, true},
                                        a2::Dimension{32, 0.0f, 1.0f, false});

        lut_out.write("ggx_roughness_mu");

        display = AI_RGB_GREEN;
    }

    sg->out.CLOSURE() = AiClosureEmission(sg, display);
}

node_loader {
    if (i > 0)
        return false;

    node->methods = A2IntegratorMtd;
    node->output_type = AI_TYPE_CLOSURE;
    node->name = "a2_integrator";
    node->node_type = AI_NODE_SHADER;
    strcpy(node->version, AI_VERSION);
    return true;
}
