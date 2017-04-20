#include <ai.h>
#include <cstring>
#include <string>
#include "cryptomatte.h"

AI_SHADER_NODE_EXPORT_METHODS(cryptomatte_aovMtd)

enum cryptomatte_aovParams
{
   p_aov_crypto_asset,
   p_aov_crypto_object,
   p_aov_crypto_material
};

node_parameters
{
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
   if (sg->Rt & AI_RAY_CAMERA && sg->Op != NULL) {
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
