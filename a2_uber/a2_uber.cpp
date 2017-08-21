#include "bsdf_diffuse.hpp"
#include "bsdf_microfacet.hpp"
#include "bsdf_microfacet_refraction.hpp"
#include "bsdf_stack.hpp"
#include "common/util.hpp"
#include <ai.h>
#include <iostream>
#include <spdlog/fmt/ostr.h>

#include "lut_ggx_roughness_mu.hpp"

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

    auto roughness_lut = get_lut_ggx_roughness_mu();
    // auto roughness_lut = std::make_unique<a2::LUT2D<float>>(
    // std::vector<float>{
    // 0,         0.00378937, 0.0197579, 0.0497698,  0.0929081, 0.147668,
    // 0.210491,  0.277009,   0.343854,  0.407538,   0.467402,  0.52242,
    // 0.572312,  0.617065,   0.656994,  0.692427,   0,         0.00490576,
    // 0.0217229, 0.0532586,  0.0984536, 0.154618,   0.217305,  0.282409,
    // 0.345961,  0.406814,   0.463677,  0.516067,   0.563894,  0.607031,
    // 0.645841,  0.68055,    0,         0.00508744, 0.0230061, 0.0565727,
    // 0.104098,  0.161431,   0.223303,  0.286276,   0.346253,  0.404077,
    // 0.458018,  0.508017,   0.55356,   0.595244,   0.632875,  0.666885,
    // 0,         0.00541347, 0.0246577, 0.0607381,  0.110689,  0.168654,
    // 0.229471,  0.288866,   0.345743,  0.400375,   0.451335,  0.498621,
    // 0.542234,  0.582159,   0.618592,  0.651823,   0,         0.00594246,
    // 0.0268,    0.066021,   0.118349,  0.176441,   0.234764,  0.290346,
    // 0.344069,  0.395244,   0.443223,  0.488063,   0.529417,  0.567796,
    // 0.603091,  0.635487,   0,         0.00637639, 0.0296471, 0.0726154,
    // 0.126992,  0.184231,   0.23905,   0.290803,   0.341089,  0.388805,
    // 0.433706,  0.475853,   0.515158,  0.551815,   0.585896,  0.61728,
    // 0,         0.00711524, 0.0334737, 0.0807093,  0.136975,  0.191297,
    // 0.24172,   0.289893,   0.336321,  0.380481,   0.422226,  0.461778,
    // 0.499015,  0.534085,   0.566733,  0.597165,   0,         0.00810128,
    // 0.0387674, 0.0907984,  0.146942,  0.197129,   0.24298,   0.287081,
    // 0.329427,  0.370093,   0.408704,  0.445692,   0.480733,  0.513929,
    // 0.545231,  0.574651,   0,         0.00958961, 0.0461581, 0.103483,
    // 0.156084,  0.200926,   0.242371,  0.281939,   0.3202,    0.357101,
    // 0.392672,  0.427019,   0.459922,  0.49129,    0.521098,  0.549319,
    // 0,         0.0118201,  0.0565292, 0.116551,   0.163568,  0.202776,
    // 0.239112,  0.273843,   0.307899,  0.341179,   0.373692,  0.405266,
    // 0.435856,  0.465362,   0.493573,  0.520419,   0,         0.0155655,
    // 0.0722423, 0.129005,   0.168353,  0.201546,   0.23229,   0.262128,
    // 0.291856,  0.321511,   0.351018,  0.379862,   0.408193,  0.435498,
    // 0.461887,  0.487212,   0,         0.0220886,  0.0910754, 0.138478,
    // 0.169687,  0.195996,   0.220949,  0.246017,   0.271701,  0.297647,
    // 0.323781,  0.349789,   0.375426,  0.400511,   0.424867,  0.448332,
    // 0,         0.0347369,  0.109612,  0.143653,   0.165401,  0.18438,
    // 0.203731,  0.224147,   0.245736,  0.268147,   0.290998,  0.31388,
    // 0.336644,  0.358914,   0.380801,  0.402477,   0,         0.0621111,
    // 0.122265,  0.14085,    0.152342,  0.163942,   0.177788,  0.193845,
    // 0.211502,  0.230234,   0.249531,  0.269057,   0.288608,  0.307973,
    // 0.327095,  0.345744,   0,         0.0983706,  0.122704,  0.122908,
    // 0.124335,  0.130402,   0.140217,  0.152652,   0.166555,  0.181349,
    // 0.196641,  0.212176,   0.227755,  0.24335,    0.258811,  0.273922,
    // 0,         0.111836,   0.0887908, 0.0768739,  0.0760823, 0.0801937,
    // 0.086946,  0.0953847,  0.104608,  0.114391,   0.124502,  0.134637,
    // 0.144893,  0.155245,   0.165622,  0.17574},
    // a2::Dimension{16, 0.0f, 1.0f, true},
    // a2::Dimension{16, 0.0f, 1.0f, false});

    float roughness = AiShaderEvalParamFlt(0);

    auto bsdf_microfacet_wrap = a2::BsdfMicrofacet::create(
        sg, AtRGB(1), sg->Nf, sg->dPdu, 1.0f, 1.5f, roughness, roughness);

    auto bsdf_microfacet_refraction = a2::BsdfMicrofacetRefraction::create(
        sg, AtRGB(1), sg->Nf, sg->dPdu, 1.0f, 1.5f, 0, 0);

    float ms_compensation =
        roughness_lut->lookup(roughness, 1.0f - a2::dot(sg->Nf, -sg->Rd));

    auto bsdf_oren_nayar =
        a2::BsdfDiffuse::create(sg, AtRGB(0.18f), sg->Nf, sg->dPdu, 0.0f);

    auto bsdf_stack = a2::BsdfStack::create(sg);
    bsdf_stack->add_bsdf(bsdf_microfacet_wrap);
    // bsdf_stack->add_bsdf(bsdf_oren_nayar);
    // bsdf_stack->add_bsdf(bsdf_microfacet_refraction);
    // sg->out.CLOSURE() = bsdf_stack->get_arnold_bsdf();
    auto clist = AtClosureList();
    clist.add(bsdf_microfacet_wrap->get_arnold_bsdf());
    clist.add(AiOrenNayarBSDF(sg, AtRGB(ms_compensation), sg->Nf));
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
