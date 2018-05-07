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
    p_preview_in_exr,
    p_process_maya,
    p_process_paths,
    p_process_obj_path_pipes,
    p_process_mat_path_pipes,
    p_process_legacy,
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
    AiParameterBool("preview_in_exr", CRYPTO_PREVIEWINEXR_DEFAULT);
    AiParameterBool("process_maya", true);
    AiParameterBool("process_paths", true);
    AiParameterBool("process_obj_path_pipes", true);
    AiParameterBool("process_mat_path_pipes", true);
    AiParameterBool("process_legacy", true);
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
    data->set_option_channels(AiNodeGetInt(node, "cryptomatte_depth"), AiNodeGetBool(node, "preview_in_exr"));

    CryptoNameFlag flags = CRYPTO_NAME_ALL;
    if (!AiNodeGetBool(node, "process_maya"))
        flags ^= CRYPTO_NAME_MAYA;
    if (!AiNodeGetBool(node, "process_paths"))
        flags ^= CRYPTO_NAME_PATHS;
    if (!AiNodeGetBool(node, "process_obj_path_pipes"))
        flags ^= CRYPTO_NAME_OBJPATHPIPES;
    if (!AiNodeGetBool(node, "process_mat_path_pipes"))
        flags ^= CRYPTO_NAME_MATPATHPIPES;
    if (!AiNodeGetBool(node, "process_legacy"))
        flags ^= CRYPTO_NAME_LEGACY;

    CryptoNameFlag obj_flags = flags, mat_flags = flags;
    if (!AiNodeGetBool(node, "strip_obj_namespaces"))
        obj_flags ^= CRYPTO_NAME_STRIP_NS;
    if (!AiNodeGetBool(node, "strip_mat_namespaces"))
        mat_flags ^= CRYPTO_NAME_STRIP_NS;

    data->set_option_namespace_stripping(obj_flags, mat_flags);

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
    sg->out.RGBA() = AI_RGBA_ZERO;
}

void registerCryptomatte(AtNodeLib* node) {
    node->methods = cryptomatteMtd;
    node->output_type = AI_TYPE_RGBA;
    node->name = "cryptomatte";
    node->node_type = AI_NODE_SHADER;
    strcpy(node->version, AI_VERSION);
}
