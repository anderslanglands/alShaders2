#include <ai.h>

/*
API Documentation: 

This no longer needs to be built into every shader as in Arnold 4, thanks to AOV shaders. If an alternate AOV shader
is made, it should should still interact with cryptomatte entirely through the CryptomatteData struct. 
How to add cryptomatte to a shader: 

1. Add a member of type *CryptomatteData to your ShaderData

        CryptomatteData *data = CryptomatteData_new();

2. in node_init:

        CryptomatteData_del(data);

    The constructor sets up getting the global state of things, including getting global
    options declared on the Arnold options. 

3. in node_finish:

        CryptomatteData_del(data);

4. in node_update:
        
        # set cryptomatte depth (optional)
        CryptomatteData_set_option_depth(data, AiNodeGetInt(node, "cryptomatte_depth"));
        # set namespace options (optional)
        CryptomatteData_set_option_namespace_stripping(data, AiNodeGetBool(node, "strip_obj_namespaces"), 
                                                      AiNodeGetBool(node, "strip_mat_namespaces"));
        # set option for sidecar manifest (optional)
        CryptomatteData_set_manifest_sidecar(data, sidecar);

        AtArray* uc_aov_array = AiArray(4, 1, AI_TYPE_STRING, 
            AiNodeGetStr(node, "user_crypto_aov_0"), AiNodeGetStr(node, "user_crypto_aov_1"), 
            AiNodeGetStr(node, "user_crypto_aov_2"), AiNodeGetStr(node, "user_crypto_aov_3"));
        AtArray* uc_src_array = AiArray(4, 1, AI_TYPE_STRING, 
            AiNodeGetStr(node, "user_crypto_src_0"), AiNodeGetStr(node, "user_crypto_src_1"), 
            AiNodeGetStr(node, "user_crypto_src_2"), AiNodeGetStr(node, "user_crypto_src_3"));
       
        # does all the setup work. User cryptomatte arrays are optional (can be nulls)
        CryptomatteData_setup_all(data, AiNodeGetStr(node, "aov_crypto_asset"),
                                  AiNodeGetStr(node, "aov_crypto_object"),
                                  AiNodeGetStr(node, "aov_crypto_material"), 
                                  uc_aov_array, uc_src_array);

    The three arguments are the names of the cryptomatte AOVs. If the AOVs are activ (connected to EXR drivers), 
    this does all the complicated setup work of creating multiple AOVs if necessary, writing metadata, etc. 

5. in shader_evaluate:

        CryptomatteData_do_cryptomattes(data, sg);

    This does all the hashing and writing values to AOVs, including user-defined cryptomattes. 
    Opacity does not need to be supplied.  

*/

///////////////////////////////////////////////
//
//      Constants
//
///////////////////////////////////////////////

//      For system controls
#define CRYPTO_DEPTH_DEFAULT 6
#define CRYPTO_STRIPOBJNS_DEFAULT true
#define CRYPTO_STRIPMATNS_DEFAULT true
#define CRYPTO_ICEPCLOUDVERB_DEFAULT 1
#define CRYPTO_SIDECARMANIFESTS_DEFAULT false
#define CRYPTO_PREVIEWINEXR_DEFAULT true



///////////////////////////////////////////////
//
//      Forward declarations
//
///////////////////////////////////////////////

struct CryptomatteData;

CryptomatteData* CryptomatteData_new();
void CryptomatteData_setup_all(CryptomatteData *data, const AtString aov_cryptoasset, 
                               const AtString aov_cryptoobject, const AtString aov_cryptomaterial,
                               AtArray *uc_aov_array, AtArray *uc_src_array);
void CryptomatteData_set_option_sidecar_manifests(CryptomatteData *data, bool sidecar);
void CryptomatteData_set_option_channels(CryptomatteData *data, int depth, bool previewchannels);
void CryptomatteData_set_option_namespace_stripping(CryptomatteData *data, bool strip_obj, bool strip_mat);
void CryptomatteData_set_option_ice_pcloud_verbosity(CryptomatteData *data, int verbosity);
void CryptomatteData_set_manifest_sidecar(CryptomatteData *data, bool sidecar);
void CryptomatteData_do_cryptomattes(CryptomatteData *data, AtShaderGlobals *sg );
void CryptomatteData_write_sidecar_manifests(CryptomatteData *data);
void CryptomatteData_del(CryptomatteData *data);
