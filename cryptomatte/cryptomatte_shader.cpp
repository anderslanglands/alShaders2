#include "cryptomatte.h"
#include "cryptomatte_tests.h"
#include <ai.h>
#include <cstring>
#include <string>

AI_SHADER_NODE_EXPORT_METHODS(cryptomatteMtd)

enum cryptomatteParams {
    p_sidecar_manifests,
    p_cryptomatte_depth,
    p_strip_obj_namespaces,
    p_strip_mat_namespaces,
    p_aov_crypto_asset,
    p_aov_crypto_object,
    p_aov_crypto_material,
    p_user_crypto_aov_0,
    p_user_crypto_src_0,
    p_user_crypto_aov_1,
    p_user_crypto_src_1,
    p_user_crypto_aov_2,
    p_user_crypto_src_2,
    p_user_crypto_aov_3,
    p_user_crypto_src_3,
};

node_parameters {
    AiParameterBool("sidecar_manifests", CRYPTO_SIDECARMANIFESTS_DEFAULT);
    AiParameterInt("cryptomatte_depth", CRYPTO_DEPTH_DEFAULT);
    AiParameterBool("strip_obj_namespaces", CRYPTO_STRIPOBJNS_DEFAULT);
    AiParameterBool("strip_mat_namespaces", CRYPTO_STRIPMATNS_DEFAULT);
    AiParameterStr("aov_crypto_asset", "crypto_asset");
    AiParameterStr("aov_crypto_object", "crypto_object");
    AiParameterStr("aov_crypto_material", "crypto_material");
    AiParameterStr("user_crypto_aov_0", "");
    AiParameterStr("user_crypto_src_0", "");
    AiParameterStr("user_crypto_aov_1", "");
    AiParameterStr("user_crypto_src_1", "");
    AiParameterStr("user_crypto_aov_2", "");
    AiParameterStr("user_crypto_src_2", "");
    AiParameterStr("user_crypto_aov_3", "");
    AiParameterStr("user_crypto_src_3", "");
}

node_initialize {
    CryptomatteData* data = new CryptomatteData();
    run_all_unit_tests(node);
    AiNodeSetLocalData(node, data);
}

node_finish {
    CryptomatteData* data = reinterpret_cast<CryptomatteData*>(AiNodeGetLocalData(node));
    delete data;
}

node_update {
    CryptomatteData* data = reinterpret_cast<CryptomatteData*>(AiNodeGetLocalData(node));

    data->set_option_sidecar_manifests(AiNodeGetBool(node, "sidecar_manifests"));
    data->set_option_channels(AiNodeGetInt(node, "cryptomatte_depth"), CRYPTO_PREVIEWINEXR_DEFAULT);
    data->set_option_namespace_stripping(AiNodeGetBool(node, "strip_obj_namespaces"),
                                         AiNodeGetBool(node, "strip_mat_namespaces"));

    AtArray* uc_aov_array = AiArray(4, 1, AI_TYPE_STRING, //
                                    AiNodeGetStr(node, "user_crypto_aov_0").c_str(),
                                    AiNodeGetStr(node, "user_crypto_aov_1").c_str(),
                                    AiNodeGetStr(node, "user_crypto_aov_2").c_str(),
                                    AiNodeGetStr(node, "user_crypto_aov_3").c_str());
    AtArray* uc_src_array = AiArray(4, 1, AI_TYPE_STRING, //
                                    AiNodeGetStr(node, "user_crypto_src_0").c_str(),
                                    AiNodeGetStr(node, "user_crypto_src_1").c_str(),
                                    AiNodeGetStr(node, "user_crypto_src_2").c_str(),
                                    AiNodeGetStr(node, "user_crypto_src_3").c_str());

    data->setup_all(AiNodeGetStr(node, "aov_crypto_asset"), AiNodeGetStr(node, "aov_crypto_object"),
                    AiNodeGetStr(node, "aov_crypto_material"), uc_aov_array, uc_src_array);
}

shader_evaluate {
    if (sg->Rt & AI_RAY_CAMERA && sg->sc == AI_CONTEXT_SURFACE) {
        CryptomatteData* data = reinterpret_cast<CryptomatteData*>(AiNodeGetLocalData(node));
        data->do_cryptomattes(sg);
    }
}

void registerCryptomatte(AtNodeLib* node) {
    node->methods = cryptomatteMtd;
    node->output_type = AI_TYPE_RGBA;
    node->name = "cryptomatte";
    node->node_type = AI_NODE_SHADER;
    strcpy(node->version, AI_VERSION);
}
