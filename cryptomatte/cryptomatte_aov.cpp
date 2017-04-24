#include <ai.h>
#include <cstring>
#include <string>
#include "cryptomatte.h"

AI_SHADER_NODE_EXPORT_METHODS(cryptomatte_aovMtd)

enum cryptomatte_aovParams
{
   p_cryptomatte_depth,
   p_strip_obj_namespaces,
   p_strip_mat_namespaces,
   p_aov_crypto_asset,
   p_aov_crypto_object,
   p_aov_crypto_material,
   p_user_aov_0,
   p_user_src_0,
   p_user_aov_1,
   p_user_src_1,
   p_user_aov_2,
   p_user_src_2,
   p_user_aov_3,
   p_user_src_3,
};

node_parameters
{
   AiParameterInt("cryptomatte_depth", CRYPTO_DEPTH_DEFAULT);
   AiParameterBool("strip_obj_namespaces", CRYPTO_STRIPOBJNS_DEFAULT);
   AiParameterBool("strip_mat_namespaces", CRYPTO_STRIPMATNS_DEFAULT);
   AiParameterStr("aov_crypto_asset", "crypto_asset");
   AiParameterStr("aov_crypto_object", "crypto_object");
   AiParameterStr("aov_crypto_material", "crypto_material");
   AiParameterStr("user_aov_0", "");
   AiParameterStr("user_src_0", "");
   AiParameterStr("user_aov_1", "");
   AiParameterStr("user_src_1", "");
   AiParameterStr("user_aov_2", "");
   AiParameterStr("user_src_2", "");
   AiParameterStr("user_aov_3", "");
   AiParameterStr("user_src_3", "");
}

node_initialize
{
   AiNodeSetLocalData(node, new CryptomatteData());
}

node_finish
{
   CryptomatteData *cryptomatte = (CryptomatteData*) AiNodeGetLocalData(node);
   delete cryptomatte;
}

node_update
{
   CryptomatteData *cryptomatte = (CryptomatteData*) AiNodeGetLocalData(node);

   cryptomatte->set_option_depth(AiNodeGetInt(node, "cryptomatte_depth"));
   cryptomatte->set_option_namespace_stripping(AiNodeGetBool(node, "strip_obj_namespaces"), 
                                               AiNodeGetBool(node, "strip_mat_namespaces"));

   AtArray* uc_aov_array = AiArray(4, 1, AI_TYPE_STRING, 
      AiNodeGetStr(node, "user_aov_0"), AiNodeGetStr(node, "user_aov_1"), 
      AiNodeGetStr(node, "user_aov_2"), AiNodeGetStr(node, "user_aov_3"));
   AtArray* uc_src_array = AiArray(4, 1, AI_TYPE_STRING, 
      AiNodeGetStr(node, "user_src_0"), AiNodeGetStr(node, "user_src_1"), 
      AiNodeGetStr(node, "user_src_2"), AiNodeGetStr(node, "user_src_3"));
   
   cryptomatte->setup_all(AiNodeGetStr(node, "aov_crypto_asset"),
                          AiNodeGetStr(node, "aov_crypto_object"),
                          AiNodeGetStr(node, "aov_crypto_material"), 
                          uc_aov_array, uc_src_array);
}

shader_evaluate
{
   if (sg->Rt & AI_RAY_CAMERA && sg->sc == AI_CONTEXT_SURFACE) {
      CryptomatteData *cryptomatte = (CryptomatteData*) AiNodeGetLocalData(node);
      cryptomatte->do_cryptomattes(sg);
   }
}

node_loader
{
   if (i == 0)
   {
      node->methods = cryptomatte_aovMtd;
      node->output_type = AI_TYPE_CLOSURE;
      node->name = "cryptomatte_aov";
      node->node_type = AI_NODE_SHADER;
      strcpy(node->version, AI_VERSION);
      return true;
   }
   else
   {
      return false;
   }
}
