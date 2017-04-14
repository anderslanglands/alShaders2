#include <ai.h>
#include <cstring>
#include <string>
#include "cryptomatte.h"

AI_SHADER_NODE_EXPORT_METHODS(cryptomatte_aovMtd)

enum cryptomatte_aovParams
{
   p_passthrough = 0,
   p_crypto_asset_override,
   p_crypto_object_override,
   p_crypto_material_override,
   p_aov_crypto_asset,
   p_aov_crypto_object,
   p_aov_crypto_material
};

node_parameters
{
   AiParameterRGB("passthrough", 0.0f, 0.0f, 0.0f);
   
   AiParameterStr("crypto_asset_override", "");
   AiParameterStr("crypto_object_override", "");
   AiParameterStr("crypto_material_override", "");
   AiParameterStr("aov_crypto_asset", "crypto_asset");
   AiParameterStr("aov_crypto_object", "crypto_object");
   AiParameterStr("aov_crypto_material", "crypto_material");
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
   
   cryptomatte->setup_all(AiNodeGetStr(node, "aov_crypto_asset"),
                          AiNodeGetStr(node, "aov_crypto_object"),
                          AiNodeGetStr(node, "aov_crypto_material"));
}

shader_evaluate
{
   CryptomatteData *cryptomatte = (CryptomatteData*) AiNodeGetLocalData(node);
   
   sg->out.RGB = AiShaderEvalParamRGB(p_passthrough);
   
   cryptomatte->do_cryptomattes(sg, node,
                                p_crypto_asset_override,
                                p_crypto_object_override,
                                p_crypto_material_override);
}

node_loader
{
   if (i == 0)
   {
      node->methods = cryptomatte_aovMtd;
      node->output_type = AI_TYPE_RGB;
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
