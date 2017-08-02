#include <ai.h>
#include <cstring>
#include <string>
#include "cryptomatte.h"

AI_SHADER_NODE_EXPORT_METHODS(cryptomatteMtd)

enum cryptomatteParams
{
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

node_parameters
{
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

node_initialize
{
   CryptomatteData *data = CryptomatteData_new();
   AiNodeSetLocalData(node, data);
}

node_finish
{
   CryptomatteData *data = (CryptomatteData*) AiNodeGetLocalData(node);
   CryptomatteData_del(data);
}

node_update
{
   CryptomatteData *data = (CryptomatteData*) AiNodeGetLocalData(node);

   CryptomatteData_set_option_sidecar_manifests(data, AiNodeGetBool(node, "sidecar_manifests"));
   CryptomatteData_set_option_depth(data, AiNodeGetInt(node, "cryptomatte_depth"));
   CryptomatteData_set_option_namespace_stripping(data, AiNodeGetBool(node, "strip_obj_namespaces"), 
                                                  AiNodeGetBool(node, "strip_mat_namespaces"));

   AtArray* uc_aov_array = AiArray(4, 1, AI_TYPE_STRING, 
      AiNodeGetStr(node, "user_crypto_aov_0").c_str(), AiNodeGetStr(node, "user_crypto_aov_1").c_str(), 
      AiNodeGetStr(node, "user_crypto_aov_2").c_str(), AiNodeGetStr(node, "user_crypto_aov_3").c_str());
   AtArray* uc_src_array = AiArray(4, 1, AI_TYPE_STRING, 
      AiNodeGetStr(node, "user_crypto_src_0").c_str(), AiNodeGetStr(node, "user_crypto_src_1").c_str(), 
      AiNodeGetStr(node, "user_crypto_src_2").c_str(), AiNodeGetStr(node, "user_crypto_src_3").c_str());
   
   CryptomatteData_setup_all(data, AiNodeGetStr(node, "aov_crypto_asset"),
                          AiNodeGetStr(node, "aov_crypto_object"),
                          AiNodeGetStr(node, "aov_crypto_material"), 
                          uc_aov_array, uc_src_array);
}

shader_evaluate
{
   if (sg->Rt & AI_RAY_CAMERA && sg->sc == AI_CONTEXT_SURFACE) {
      CryptomatteData *data = (CryptomatteData*) AiNodeGetLocalData(node);
      CryptomatteData_do_cryptomattes(data, sg);
   }
}

void registerCryptomatte(AtNodeLib *node)
{
   node->methods = cryptomatteMtd;
   node->output_type = AI_TYPE_CLOSURE;
   node->name = "cryptomatte";
   node->node_type = AI_NODE_SHADER;
   strcpy(node->version, AI_VERSION);
}
