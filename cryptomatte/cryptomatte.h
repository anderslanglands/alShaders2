#include <ai.h>
#include <string>
#include <unordered_set>
#include <cstring>
#include <map>
#include <ctime>
#include "MurmurHash3.h"
#include <cstdio>
#include <limits>
#include <algorithm>

#define NOMINMAX // lets you keep using std::min


/*

API Documentation: 

Shaders should interact with cryptomatte entirely through the CryptomatteData struct. 
How to add cryptomatte to a shader: 

1. Add a member of type *CryptomatteData to your ShaderData

        CryptomatteData *cryptomatte;

2. in node_init:

        data->cryptomatte = new CryptomatteData();

    The constructor sets up getting the global state of things, including getting global
    options declared on the Arnold options. 

3. in node_finish:

        delete data->cryptomatte;

    CryptomatteData's destructors clean up all allocations.

4. in node_update:

        data->cryptomatte->setup_all(AiNodeGetStr(node, "aov_cryptoasset"), 
            AiNodeGetStr(node, "aov_cryptoobject"), AiNodeGetStr(node, "aov_cryptomaterial"));

    The three arguments are the names of the cryptomatte AOVs. If the AOVs are activ (connected to EXR drivers), 
    this does all the complicated setup work of creating multiple AOVs if necessary, writing metadata, etc. 

5. in shader_evaluate:

        data->cryptomatte->do_cryptomattes(sg, node, p_override_asset, p_override_object, p_override_material, opacity);

    This does all the hashing and writing values to AOVs, including user-defined cryptomattes. 
    You pass in your AtShaderGlobals, node, as well as the enum values for the parameters 
    for the cryptomatte string overrides. If you don't provide string overrides to your users, 
    you can set those to -1. 

    Opacity must also be supplied as a float (rgb opacity is not supported). Opacity can be 
    computed from closures.  

*/

///////////////////////////////////////////////
//
//      Constants
//
///////////////////////////////////////////////

// System values
#define MAX_STRING_LENGTH 255
#define MAX_CRYPTOMATTE_DEPTH 99
#define MAX_USER_CRYPTOMATTES 16

// User data names
static const AtString CRYPTO_ASSET_UDATA("crypto_asset");
static const AtString CRYPTO_OBJECT_UDATA("crypto_object");
static const AtString CRYPTO_MATERIAL_UDATA("crypto_material");

static const AtString CRYPTO_ASSET_OFFSET_UDATA("crypto_asset_offset");
static const AtString CRYPTO_OBJECT_OFFSET_UDATA("crypto_object_offset");
static const AtString CRYPTO_MATERIAL_OFFSET_UDATA("crypto_material_offset");
// Arnold options parameters

//      For system controls
#define CRYPTO_DEPTH_DEFAULT 6
#define CRYPTO_STRIPOBJNS_DEFAULT true
#define CRYPTO_STRIPMATNS_DEFAULT true
#define CRYPTO_ICEPCLOUDVERB_DEFAULT 1

// Internal
#define CRYPTOMATTE_METADATA_SET_FLAG "already_has_crypto_metadata_"

unsigned char g_pointcloud_instance_verbosity = 0;  // to do: remove this.

// Some static AtStrings to cache
const static AtString aStr_polymesh("polymesh");
const static AtString aStr_curves("curves");
const static AtString aStr_shidxs("shidxs");
const static AtString aStr_shader("shader");

///////////////////////////////////////////////
//
//      Cryptomatte Cache
//
///////////////////////////////////////////////

#define CACHE_LINE  64
#if defined(_WIN32) || defined(_MSC_VER)
#define CACHE_ALIGN __declspec(align(CACHE_LINE))
#else
#define CACHE_ALIGN __attribute__((aligned(CACHE_LINE)))
#endif

struct CACHE_ALIGN CryptomatteCache {
    AtNode * object;        
    AtRGB nsp_hash_clr;   
    AtRGB obj_hash_clr;   
    AtNode * shader_object; 
    AtRGB mat_hash_clr;   


    CryptomatteCache() {
        this->object = NULL;
        this->nsp_hash_clr = AI_RGB_BLACK;
        this->obj_hash_clr = AI_RGB_BLACK;
        this->shader_object = NULL;
        this->mat_hash_clr = AI_RGB_BLACK;
    }
};


CryptomatteCache CRYPTOMATTE_CACHE[AI_MAX_THREADS];
void init_cryptomatte_cache() {
    for ( uint16_t i=0; i<AI_MAX_THREADS; i++) {
        CRYPTOMATTE_CACHE[i] = CryptomatteCache();
    }
}


///////////////////////////////////////////////
//
//      String processing
//
///////////////////////////////////////////////


void safe_copy_to_buffer(char buffer[MAX_STRING_LENGTH], const char* c) {
    if (c != NULL) {
        size_t length = std::min( strlen(c), (size_t) MAX_STRING_LENGTH-1);
        strncpy(buffer, c, length);
    } else {
        buffer[0] = '\0';
    }
}


bool string_has_content(const char * c) {
    return c != NULL && c[0] != '\0';
}


///////////////////////////////////////////////
//
//      Name processing
//
///////////////////////////////////////////////


bool sitoa_pointcloud_instance_handling(const char *obj_full_name, char obj_name_out[MAX_STRING_LENGTH]) {
    if (g_pointcloud_instance_verbosity == 0 || strstr(obj_full_name, ".SItoA.Instance.") == NULL)  {
        return false;
    }    
    char obj_name[MAX_STRING_LENGTH];
    safe_copy_to_buffer(obj_name, obj_full_name);

    char *instance_start = strstr(obj_name, ".SItoA.Instance.");

    char *space = strstr(instance_start, " ");
    if (space == NULL) { return false; }

    char *instance_name = &space[1];    
    char *obj_suffix2 = strstr(instance_name, ".SItoA."); 
    if (obj_suffix2 == NULL) { return false; }  
    obj_suffix2[0] = '\0';  // strip the suffix
    size_t chars_to_copy = strlen(instance_name);
    if (chars_to_copy >= MAX_STRING_LENGTH || chars_to_copy == 0) { return false; } 
    if (g_pointcloud_instance_verbosity == 2)   {
        char *frame_numbers = &instance_start[16]; // 16 chars in ".SItoA.Instance.", this gets us to the first number
        char *instance_ID = strstr(frame_numbers, ".");
        char *instance_ID_end = strstr(instance_ID, " ");
        instance_ID_end[0] = '\0';
        size_t ID_len = strlen(instance_ID);
        strncpy(&instance_name[chars_to_copy], instance_ID, ID_len);
        chars_to_copy += ID_len;
    }

    strncpy(obj_name_out, instance_name, chars_to_copy);    
    return true;
}


void get_clean_object_name(const char *obj_full_name, char obj_name_out[MAX_STRING_LENGTH], 
                           char nsp_name_out[MAX_STRING_LENGTH], bool strip_obj_ns) 
{ 
    char nsp_name[MAX_STRING_LENGTH] = "";
    safe_copy_to_buffer(nsp_name, obj_full_name);
    bool preempt_object_name = false;

    char *obj_postfix = strstr(nsp_name, ".SItoA.");
    if (obj_postfix != NULL) {
        // in Softimage mode
        // to do: when there are more than one way to preempt object names here, we're going to have to have some kind of loop handling that. 
        preempt_object_name = sitoa_pointcloud_instance_handling(obj_full_name, obj_name_out);
        obj_postfix[0] = '\0';
    }

    char *space_finder = strstr(nsp_name, " ");
    while (space_finder != NULL) {
        space_finder[0] = '#';
        space_finder = strstr(nsp_name, " ");
    }

    char *nsp_separator = strchr(nsp_name, ':');
    if (nsp_separator == NULL) {
        nsp_separator = strchr(nsp_name, '.');
    }

    if (nsp_separator != NULL) {
        if (strip_obj_ns) {
            if (!preempt_object_name) {
                char *obj_name_start = nsp_separator + 1;
                memmove(obj_name_out, obj_name_start, strlen(obj_name_start));
            }
        } else {
            if (!preempt_object_name)
                memmove(obj_name_out, nsp_name, strlen(nsp_name)); // the object name is the model name in this case
        }
        nsp_separator[0] = '\0';
    } else {
        // no namespace
        if (!preempt_object_name)
            memmove(obj_name_out, nsp_name, strlen(nsp_name)); // the object name is the model name in this case
        strncpy(nsp_name, "default\0", 8); // and the model name is default. 
    }

    strcpy(nsp_name_out, nsp_name);
}


void get_clean_material_name(const char *mat_full_name, char mat_name_out[MAX_STRING_LENGTH], bool strip_ns) {    
    // Example: 
    //      Softimage: Sources.Materials.myLibrary_ref_library.myMaterialName.Standard_Mattes.uBasic.SITOA.25000....
    //      Maya: namespace:my_material_sg
    safe_copy_to_buffer(mat_name_out, mat_full_name);

    char *mat_postfix = strstr(mat_name_out, ".SItoA.");
    if (mat_postfix != NULL) {
        //   Sources.Materials.myLibrary_ref_library.myMaterialName.Standard_Mattes.uBasic  <<chop>> .SITOA.25000....
        mat_postfix[0] = '\0';

        char *mat_shader_name = strrchr(mat_name_out, '.');
        if (mat_shader_name != NULL) {
            //   Sources.Materials.myLibrary_ref_library.myMaterialName.Standard_Mattes <<chop>> .uBasic 
            mat_shader_name[0] = '\0';
        }

        char *Standard_Mattes = strstr(mat_name_out, ".Standard_Mattes");
        if (Standard_Mattes != NULL)
            //   Sources.Materials.myLibrary_ref_library.myMaterialName <<chop>> .Standard_Mattes
            Standard_Mattes[0] = '\0';

        const char * prefix = "Sources.Materials.";
        const char *mat_prefix_seperator = strstr(mat_name_out, prefix);
        if (mat_prefix_seperator != NULL) {
            //   Sources.Materials. <<SNIP>> myLibrary_ref_library.myMaterialName
            const char *mat_name_start = mat_prefix_seperator + strlen(prefix) ;
            memmove(mat_name_out, mat_name_start, strlen(mat_name_start) + 1);
        }

        char *nsp_separator = strchr(mat_name_out, '.');
        if (strip_ns && nsp_separator != NULL) {
            //   myLibrary_ref_library. <<SNIP>> myMaterialName
            nsp_separator[0] = '\0';
            char *mat_name_start = nsp_separator + 1;
            memmove(mat_name_out, mat_name_start, strlen(mat_name_start) + 1);
        } 
        // Leaving us with just the material name. 
    }

    // For maya, you get something simpler, like namespace:my_material_sg.
    char *ns_separator = strchr(mat_name_out, ':');
    if (strip_ns && ns_separator != NULL) {
        //    namespace: <<SNIP>> my_material_sg
        ns_separator[0] = '\0';
        char *mat_name_start = ns_separator + 1;
        memmove(mat_name_out, mat_name_start, strlen(mat_name_start) + 1);
    } 
}


float hash_to_float(uint32_t hash) {
    uint32_t mantissa = hash & (( 1 << 23) - 1);
    uint32_t exponent = (hash >> 23) & ((1 << 8) - 1);
    exponent = std::max(exponent, (uint32_t) 1);
    exponent = std::min(exponent, (uint32_t) 254);
    exponent = exponent << 23;
    uint32_t sign = (hash >> 31);
    sign = sign << 31;
    uint32_t float_bits = sign | exponent | mantissa;
    float f;
    std::memcpy(&f, &float_bits, 4);
    return f;
}


void hash_name_rgb(const char * name, AtRGB* out_color) {
    // This puts the float ID into the red channel, and the human-readable
    // versions into the G and B channels. 
    uint32_t m3hash = 0;
    MurmurHash3_x86_32(name, (uint32_t) strlen(name), 0, &m3hash);
    out_color->r = hash_to_float(m3hash);
    out_color->g = ((float) ((m3hash << 8)) /  (float) std::numeric_limits<uint32_t>::max());
    out_color->b = ((float) ((m3hash << 16)) / (float) std::numeric_limits<uint32_t>::max());
}


AtString get_user_data(const AtShaderGlobals * sg, const AtNode * node, const AtString user_data_name, 
                       bool *cachable) {
    // returns the string if the parameter is usable, modifies cachable
    const AtUserParamEntry* pentry = AiNodeLookUpUserParameter(node, user_data_name);
    if (pentry) {
        if (AiUserParamGetType(pentry) == AI_TYPE_STRING && AiUserParamGetCategory(pentry) == AI_USERDEF_CONSTANT) {
            return AiNodeGetStr(node, user_data_name);
        } 
    }
    if (sg) {
        // this is intentionally outside the if (pentry) block. 
        // With user data declared on ginstances and such, no pentry
        // is aquirable but AiUDataGetStr still works. 
        AtString udata_value;
        if (AiUDataGetStr(user_data_name, udata_value)) {
            *cachable = false;
            return udata_value;
        }
    }
    return AtString();
}

int get_offset_user_data(const AtShaderGlobals * sg, const AtNode * node, const AtString user_data_name, 
                         bool *cachable) {
    // returns the string if the parameter is usable, modifies cachable
    const AtUserParamEntry* pentry = AiNodeLookUpUserParameter(node, user_data_name);
    if (pentry) {
        if (AiUserParamGetType(pentry) == AI_TYPE_INT && AiUserParamGetCategory(pentry) == AI_USERDEF_CONSTANT) {
            return AiNodeGetInt(node, user_data_name);
        } else if (AiUserParamGetCategory(pentry) != AI_USERDEF_CONSTANT) {
            *cachable = false;
        }
    }
    if (sg) {
        // this is intentionally outside the if (pentry) block. 
        // With user data declared on ginstances and such, no pentry
        // is aquirable but AiUDataGetStr still works. 
        int udata_value = 0;
        if (AiUDataGetInt(user_data_name, udata_value)) {
            *cachable = false;
            return udata_value;
        }
    }
    return 0;
}


void offset_name(const AtShaderGlobals *sg, const AtNode *node, const int offset, 
                 char obj_name_out[MAX_STRING_LENGTH]) {
    if (offset) {
        char offset_num_str[12];
        sprintf(offset_num_str, "_%d", offset);
        strcat(obj_name_out, offset_num_str);
    }
}

bool get_object_names(AtShaderGlobals *sg, AtNode *node, bool strip_obj_ns, 
                      char nsp_name_out[MAX_STRING_LENGTH], char obj_name_out[MAX_STRING_LENGTH]) {
    bool cachable = true;

    const AtString nsp_user_data = get_user_data(sg, node, CRYPTO_ASSET_UDATA, &cachable);
    const AtString obj_user_data = get_user_data(sg, node, CRYPTO_OBJECT_UDATA, &cachable);

    bool need_nsp_name = nsp_user_data.empty();        
    bool need_obj_name = obj_user_data.empty();
    if (need_obj_name || need_nsp_name) {
        const char *obj_full_name = AiNodeGetName(node);
        get_clean_object_name(obj_full_name, obj_name_out, nsp_name_out, strip_obj_ns);
    }

    offset_name(sg, node, get_offset_user_data(sg, node, CRYPTO_OBJECT_OFFSET_UDATA, &cachable), 
                obj_name_out);
    offset_name(sg, node, get_offset_user_data(sg, node, CRYPTO_ASSET_OFFSET_UDATA, &cachable), 
                nsp_name_out);

    if (nsp_user_data)
        strcpy(nsp_name_out, nsp_user_data);

    if (obj_user_data)
        strcpy(obj_name_out, obj_user_data);

    nsp_name_out[MAX_STRING_LENGTH-1] = '\0';
    return cachable;
}


bool get_material_name(AtShaderGlobals *sg, AtNode *node, AtNode *shader, bool strip_mat_ns, 
                       char mat_name_out[MAX_STRING_LENGTH]) 
{
    bool cachable = true;
    AtString mat_user_data = get_user_data(sg, node, CRYPTO_MATERIAL_UDATA, &cachable);

    get_clean_material_name(AiNodeGetName(shader), mat_name_out, strip_mat_ns);
    offset_name(sg, node, get_offset_user_data(sg, node, CRYPTO_MATERIAL_OFFSET_UDATA, &cachable), 
                mat_name_out);

    if (!mat_user_data.empty())
        strcpy(mat_name_out, mat_user_data); 

    mat_name_out[MAX_STRING_LENGTH-1] = '\0';
    return cachable;
}


///////////////////////////////////////////////
//
//      CRYPTOMATTE UTILITIES
//
///////////////////////////////////////////////


void write_array_of_AOVs(AtShaderGlobals * sg, AtArray * names, float id) {
    for (uint32_t i=0; i < AiArrayGetNumElements(names); i++) {
        AtString aovName = AiArrayGetStr( names, i);
        if (aovName.empty())
            return;
        AiAOVSetFlt(sg, aovName, id);
    }
}


///////////////////////////////////////////////
//
//      Metadata Writing
//
///////////////////////////////////////////////


typedef std::map<std::string,float>             manf_map_t ;
typedef std::map<std::string,float>::iterator   manf_map_it;

void compute_metadata_ID(char id_buffer[8], AtString cryptomatte_name) {
    AtRGB color_hash;
    hash_name_rgb(cryptomatte_name.c_str(), &color_hash);
    uint32_t float_bits;
    std::memcpy(&float_bits, &color_hash.r, 4);
    char hex_chars[9];
    sprintf(hex_chars, "%08x", float_bits);

    strncpy(id_buffer, hex_chars, 7);
}

void write_metadata_to_driver(AtNode * driver, AtString cryptomatte_name, manf_map_t * map) {
    if (!driver)
        return;

    AtArray * orig_md = AiNodeGetArray( driver, "custom_attributes");
    const uint32_t orig_num_entries = orig_md ? AiArrayGetNumElements(orig_md) : 0;

    std::string metadata_hash, metadata_conv, metadata_name, metadata_manf; // the new entries
    AtArray * combined_md = AiArrayAllocate(orig_num_entries + 4, 1, AI_TYPE_STRING); //Does not need destruction

    std::string prefix("STRING cryptomatte/");
    char metadata_id_buffer[8];
    compute_metadata_ID(metadata_id_buffer, cryptomatte_name);
    prefix += std::string(metadata_id_buffer) + std::string("/");

    for (uint32_t i=0; i<orig_num_entries; i++) {
        if (prefix.compare(AiArrayGetStr(orig_md, i)) == 0) {
            AiMsgWarning("Cryptomatte: Unable to write metadata. EXR metadata key, %s, already in use.", prefix.c_str());
            return;
        }
    }

    metadata_hash = prefix + std::string("hash MurmurHash3_32");
    metadata_conv = prefix + std::string("conversion uint32_to_float32");
    metadata_name = prefix + std::string("name ") + cryptomatte_name.c_str();
    metadata_manf = prefix + std::string("manifest ");

    manf_map_it map_it = map->begin();
    const size_t map_entries = map->size();
    const size_t max_entries = 100000;
    size_t metadata_entries = map_entries;
    if (map_entries > max_entries) {
        AiMsgWarning("Cryptomatte: %lu entries in manifest, limiting to %lu", map_entries, max_entries);
        metadata_entries = max_entries;
    }

    metadata_manf.append("{");
    for (uint32_t i=0; i<metadata_entries; i++) {
        const char * name = map_it->first.c_str();
        float hash_value = map_it->second;
        ++map_it;

        uint32_t float_bits;
        std::memcpy(&float_bits, &hash_value, 4);
        char hex_chars[9];
        sprintf(hex_chars, "%08x", float_bits);

        std::string pair;
        pair.append("\"");
        pair.append(name);
        pair.append("\":\"");
        pair.append(hex_chars);
        pair.append("\"");
        if (i < map_entries-1)
            pair.append(",");
        metadata_manf.append(pair);
    }
    metadata_manf.append("}");
    
    for (uint32_t i=0; i<orig_num_entries; i++) {
        AiArraySetStr(combined_md, i, AiArrayGetStr(orig_md, i));
    }
    AiArraySetStr(combined_md, orig_num_entries + 0, metadata_manf.c_str());
    AiArraySetStr(combined_md, orig_num_entries + 1, metadata_hash.c_str());
    AiArraySetStr(combined_md, orig_num_entries + 2, metadata_conv.c_str());
    AiArraySetStr(combined_md, orig_num_entries + 3, metadata_name.c_str());

    AiNodeSetArray( driver, "custom_attributes", combined_md);
}


bool metadata_needed(AtNode* driver, const AtString aov_name) {
    std::string flag = std::string(CRYPTOMATTE_METADATA_SET_FLAG) + aov_name.c_str();
    return (driver && !AiNodeLookUpUserParameter(driver, flag.c_str()));
}


void metadata_set_unneeded(AtNode* driver, const AtString aov_name) {
    if (driver == NULL)
        return;
    std::string flag = std::string(CRYPTOMATTE_METADATA_SET_FLAG) + aov_name.c_str();
    if (AiNodeLookUpUserParameter(driver, flag.c_str()) == NULL)
        AiNodeDeclare(driver, flag.c_str(), "constant BOOL");
}


void add_hash_to_map(const char * c_str, manf_map_t * md_map) {
    if (!string_has_content(c_str))
        return;
    AtRGB hash;
    std::string name_string = std::string(c_str);
    if (md_map->count(name_string) == 0) {
        hash_name_rgb(c_str, &hash);
        (*md_map)[name_string] = hash.r;
    }
}


void build_user_metadata(AtArray * uc_info, AtArray* drivers) {
    if (uc_info == NULL || drivers == NULL)
        return;

    const clock_t metadata_start_time = clock();
    bool do_metadata[MAX_USER_CRYPTOMATTES];
    manf_map_t map_md_user[MAX_USER_CRYPTOMATTES];

    AtArray * uc_aov_array = AiArrayGetArray(uc_info, 0);
    AtArray * uc_src_array = AiArrayGetArray(uc_info, 1);

    bool do_anything = false;
    for (uint32_t i=0; i<AiArrayGetNumElements(drivers); i++) {
        AtNode *driver = static_cast<AtNode*>(AiArrayGetPtr(drivers, i));
        if (driver == NULL) {
            do_metadata[i] = false;
        } else {
            do_metadata[i] = metadata_needed(driver, AiArrayGetStr(uc_aov_array, i));
            do_anything = do_anything || do_metadata[i];
        }
    }

    if (!do_anything)
        return;

    AtNodeIterator * shape_iterator = AiUniverseGetNodeIterator(AI_NODE_SHAPE);
    while (!AiNodeIteratorFinished(shape_iterator)) {
        AtNode *node = AiNodeIteratorGetNext(shape_iterator);
        for (uint32_t i=0; i<AiArrayGetNumElements(drivers); i++) {
            if (!do_metadata[i])
                continue;
            const char * user_data_name = AiArrayGetStr(uc_src_array, i);
            const AtUserParamEntry* pentry = AiNodeLookUpUserParameter(node, user_data_name);
            if (pentry == NULL|| AiUserParamGetType(pentry) != AI_TYPE_STRING)
                continue;

            if ( AiUserParamGetCategory(pentry) == AI_USERDEF_CONSTANT) {
                // not an array
                add_hash_to_map(AiNodeGetStr(node, user_data_name), &map_md_user[i]);
            } else {
                AtArray * values = AiNodeGetArray(node, user_data_name);
                if (values != NULL) {
                    for (uint32_t ai=0; ai<AiArrayGetNumElements(values); ai++)
                        add_hash_to_map(AiArrayGetStr(values, ai), &map_md_user[i]);
                }
            }
        }
    }
    AiNodeIteratorDestroy(shape_iterator);

    for (uint32_t i=0; i<AiArrayGetNumElements(drivers); i++) {
        if (!do_metadata[i])
            continue;
        
        AtNode *driver = static_cast<AtNode*>(AiArrayGetPtr(drivers, i));
        AtString aov_name = AiArrayGetStr(uc_aov_array, i);
        metadata_set_unneeded(driver, aov_name);
        write_metadata_to_driver(driver, aov_name, &map_md_user[i]);
    }
    AiMsgInfo("User Cryptomatte manifests created - %f seconds",
              float(clock() - metadata_start_time) / CLOCKS_PER_SEC);
}

void add_obj_to_manifest(const AtNode *node, char name[MAX_STRING_LENGTH], 
                          const AtString offset_user_data, manf_map_t *hash_map ) {
    {
        add_hash_to_map(name, hash_map);
        bool cachable = true;
        get_offset_user_data(NULL, node, offset_user_data, &cachable);
        if (!cachable) {
            AtArray *offsets = AiNodeGetArray(node, offset_user_data);
            if (offsets) {                    
                std::unordered_set<int> visitedOffsets;
                for (uint32_t i=0; i<AiArrayGetNumElements(offsets); i++) {
                    int offset = AiArrayGetInt(offsets, i);
                    if (visitedOffsets.find(offset) == visitedOffsets.end()) {
                        visitedOffsets.insert(offset);
                        char name_copy[MAX_STRING_LENGTH] = "";
                        safe_copy_to_buffer(name_copy, name);
                        offset_name(NULL, node, offset, name_copy);
                        add_hash_to_map(name_copy, hash_map);
                    }
                }

            }
        }
    }
}

void build_standard_metadata(AtNode* driver_asset, AtNode* driver_object, AtNode* driver_material, 
                             AtString aov_asset, AtString aov_object, AtString aov_material, 
                             bool strip_obj_ns, bool strip_mat_ns) {
    const clock_t metadata_start_time = clock();

    const bool do_md_asset = metadata_needed(driver_asset, aov_asset);
    const bool do_md_object = metadata_needed(driver_object, aov_object);
    const bool do_md_material = metadata_needed(driver_material, aov_material);

    metadata_set_unneeded(driver_asset, aov_asset);
    metadata_set_unneeded(driver_object, aov_object);
    metadata_set_unneeded(driver_material, aov_material);

    if (!do_md_asset && !do_md_object && !do_md_material)
        return;

    manf_map_t map_md_asset;
    manf_map_t map_md_object;
    manf_map_t map_md_material;

    AtNodeIterator * shape_iterator = AiUniverseGetNodeIterator(AI_NODE_SHAPE);
    while (!AiNodeIteratorFinished(shape_iterator)) {
        AtNode *node = AiNodeIteratorGetNext(shape_iterator);
        char nsp_name[MAX_STRING_LENGTH] = "";
        char obj_name[MAX_STRING_LENGTH] = "";

        get_object_names(NULL, node, strip_obj_ns, nsp_name, obj_name);

        if (do_md_asset || do_md_object) { 
            add_obj_to_manifest(node, nsp_name, CRYPTO_ASSET_OFFSET_UDATA, &map_md_asset);
            add_obj_to_manifest(node, obj_name, CRYPTO_OBJECT_OFFSET_UDATA, &map_md_object);
        }

        if (do_md_material) {
            // Process all shaders from the objects into the manifest. 
            // This includes cluster materials.
            AtArray * shaders = AiNodeGetArray(node, "shader");
            for (uint32_t i = 0; i < AiArrayGetNumElements(shaders); i++) {
                char mat_name[MAX_STRING_LENGTH] = "";
                AtNode * shader = static_cast<AtNode*>(AiArrayGetPtr(shaders, i));
                get_material_name(NULL, node, shader, strip_mat_ns, mat_name);
                add_obj_to_manifest(node, mat_name, CRYPTO_MATERIAL_OFFSET_UDATA, &map_md_material);
            }
        }
    }
    AiNodeIteratorDestroy(shape_iterator);

    write_metadata_to_driver(driver_object, aov_object, &map_md_object);
    write_metadata_to_driver(driver_material, aov_material, &map_md_material);
    write_metadata_to_driver(driver_asset, aov_asset, &map_md_asset);

    AiMsgInfo("Cryptomatte manifest created - %f seconds", (float( clock () - metadata_start_time ) /  CLOCKS_PER_SEC));
}

///////////////////////////////////////////////
//
//      Shader Data Struct
//
///////////////////////////////////////////////

struct CryptomatteData {
    uint8_t option_depth;
    uint8_t option_aov_depth;
    bool    option_strip_obj_ns;
    bool    option_strip_mat_ns;
    uint8_t option_pcloud_ice_verbosity;
    AtString aov_cryptoasset;
    AtString aov_cryptoobject;
    AtString aov_cryptomaterial;
    AtArray * aovArray_cryptoasset;
    AtArray * aovArray_cryptoobject;
    AtArray * aovArray_cryptomaterial;
    AtArray * user_cryptomatte_info;

public:
    CryptomatteData() {
        this->aovArray_cryptoasset    = NULL;
        this->aovArray_cryptoobject   = NULL;
        this->aovArray_cryptomaterial = NULL;
        this->user_cryptomatte_info   = NULL;
        init_cryptomatte_cache();
        this->set_option_depth(CRYPTO_DEPTH_DEFAULT);
        this->set_option_namespace_stripping(CRYPTO_STRIPOBJNS_DEFAULT, CRYPTO_STRIPMATNS_DEFAULT);
        this->set_option_ice_pcloud_verbosity(CRYPTO_ICEPCLOUDVERB_DEFAULT);
    }

    void setup_all(const AtString aov_cryptoasset, const AtString aov_cryptoobject, const AtString aov_cryptomaterial,
                   AtArray *uc_aov_array, AtArray *uc_src_array) {
        this->aov_cryptoasset = aov_cryptoasset;
        this->aov_cryptoobject = aov_cryptoobject;
        this->aov_cryptomaterial = aov_cryptomaterial;

        this->destroy_arrays();

        this->init_user_cryptomatte_data(uc_aov_array, uc_src_array);
        this->setup_cryptomatte_nodes();
    }

    void set_option_depth(int depth) {
        depth = std::min(std::max(depth, 1), MAX_CRYPTOMATTE_DEPTH);
        this->option_depth = depth;
        if ( this->option_depth % 2 == 0 )
            this->option_aov_depth = this->option_depth/2;
        else
            this->option_aov_depth = (this->option_depth + 1)/2;
    }

    void set_option_namespace_stripping(bool strip_obj, bool strip_mat) {
        this->option_strip_obj_ns = strip_obj;
        this->option_strip_mat_ns = strip_mat;
    }

    void set_option_ice_pcloud_verbosity(int verbosity) {
        verbosity = std::min(std::max(verbosity, 0), 2);
        this->option_pcloud_ice_verbosity = verbosity;
        g_pointcloud_instance_verbosity = this->option_pcloud_ice_verbosity; // to do: really remove this
    }

    void do_cryptomattes(AtShaderGlobals *sg ) {
        if (sg->Rt & AI_RAY_CAMERA && sg->sc == AI_CONTEXT_SURFACE) {
            this->do_standard_cryptomattes(sg);
            this->do_user_cryptomattes(sg);
        }
    }

private:
    void init_user_cryptomatte_data(const AtArray *aov_input, const AtArray *src_input) {
        /*
        Structure of user data generated is currently this. 
            this->user_cryptomatte_info[0] = aov array
            this->user_cryptomatte_info[1] = source array
            this->user_cryptomatte_info[2:] = sub-aov arrays for aov and source i-1
        Example: 
            this->user_cryptomatte_info[0] = ['myCryptoObject', 'myCryptoAsset']. 
            this->user_cryptomatte_info[1] = ['myObjUserData', 'myAssetUserData']. 
            this->user_cryptomatte_info[2] = ['myCryptoObject00', 'myCryptoObject01', 'myCryptoObject02']
            this->user_cryptomatte_info[3] = ['myCryptoAsset00', 'myCryptoAsset01', 'myCryptoAsset02']  
        */
        if (aov_input == NULL && src_input == NULL)
            return;

        const int num_inputs = std::min(AiArrayGetNumElements(aov_input), 
                                        AiArrayGetNumElements(src_input));
        AtArray *uc_aov_array = AiArrayAllocate(MAX_USER_CRYPTOMATTES, 1, AI_TYPE_STRING);
        AtArray *uc_src_array = AiArrayAllocate(MAX_USER_CRYPTOMATTES, 1, AI_TYPE_STRING);
        int uc_count = 0;
        for (uint32_t i=0; i<num_inputs; i++) {
            const AtString aov = AiArrayGetStr(aov_input, i);
            const AtString src = AiArrayGetStr(src_input, i);
            if (!aov.empty() && !src.empty()) {
                AiMsgInfo("Adding user-Cryptomatte %d: AOV: %s Source user data: %s", 
                          uc_count, aov, src);
                AiArraySetStr(uc_aov_array, uc_count, aov);
                AiArraySetStr(uc_src_array, uc_count, src);
                uc_count++;
            }
        }
        AiArrayResize(uc_aov_array, uc_count, 1);
        AiArrayResize(uc_src_array, uc_count, 1);

        if (uc_count) {
            this->user_cryptomatte_info = AiArrayAllocate(2 + uc_count, 1, AI_TYPE_ARRAY);
            AiArraySetArray(this->user_cryptomatte_info, 0, uc_aov_array);
            AiArraySetArray(this->user_cryptomatte_info, 1, uc_src_array);
            for (int i=0; i<uc_count; i++)
                AiArraySetArray(this->user_cryptomatte_info, i+2, NULL);
        } else {
            AiArrayDestroy(uc_aov_array);
            AiArrayDestroy(uc_src_array);
        }
    }

    void do_standard_cryptomattes(AtShaderGlobals *sg) {
        if (!this->aovArray_cryptoasset && !this->aovArray_cryptoobject && !this->aovArray_cryptomaterial)
            return;
        
        AtRGB nsp_hash_clr, obj_hash_clr, mat_hash_clr;
        this->hash_object_rgb(sg, &nsp_hash_clr, &obj_hash_clr, &mat_hash_clr);

        if (this->aovArray_cryptoasset)
            write_array_of_AOVs(sg, this->aovArray_cryptoasset, nsp_hash_clr.r);
        if (this->aovArray_cryptoobject)
            write_array_of_AOVs(sg, this->aovArray_cryptoobject, obj_hash_clr.r);
        if (this->aovArray_cryptomaterial)
            write_array_of_AOVs(sg, this->aovArray_cryptomaterial, mat_hash_clr.r);
        
        nsp_hash_clr.r = 0.0f;
        obj_hash_clr.r = 0.0f;
        mat_hash_clr.r = 0.0f;

        AiAOVSetRGBA(sg, this->aov_cryptoasset, nsp_hash_clr);      
        AiAOVSetRGBA(sg, this->aov_cryptoobject, obj_hash_clr);
        AiAOVSetRGBA(sg, this->aov_cryptomaterial, mat_hash_clr);
    }

    void do_user_cryptomattes(AtShaderGlobals * sg ) {
        if (this->user_cryptomatte_info == NULL)
            return;

        AtArray* uc_aov_array = AiArrayGetArray(this->user_cryptomatte_info, 0);
        AtArray* uc_src_array = AiArrayGetArray(this->user_cryptomatte_info, 1);

        for (uint32_t uc_index=2; uc_index<AiArrayGetNumElements(this->user_cryptomatte_info); uc_index++) {
            AtArray * aovArray = AiArrayGetArray(this->user_cryptomatte_info, uc_index);
            if (aovArray != NULL) {
                uint32_t i = uc_index-2;
                AtString aov_name = AiArrayGetStr(uc_aov_array, i);
                AtString src_data_name = AiArrayGetStr(uc_src_array, i);
                AtRGB hash = AI_RGB_BLACK;
                AtString result;

                AiUDataGetStr(src_data_name, result);
                if (!result.empty())
                    hash_name_rgb(result.c_str(), &hash);

                write_array_of_AOVs(sg, aovArray, hash.r);
                AiAOVSetRGBA(sg, aov_name, hash);
            }
        }
    }

    void hash_object_rgb(AtShaderGlobals* sg, AtRGB * nsp_hash_clr, AtRGB * obj_hash_clr, AtRGB * mat_hash_clr) 
    {
        if (CRYPTOMATTE_CACHE[sg->tid].object == sg->Op) {
            *nsp_hash_clr = CRYPTOMATTE_CACHE[sg->tid].nsp_hash_clr;
            *obj_hash_clr = CRYPTOMATTE_CACHE[sg->tid].obj_hash_clr;
         } else {
            char nsp_name[MAX_STRING_LENGTH] = "";
            char obj_name[MAX_STRING_LENGTH] = "";
            bool cachable = get_object_names(sg, sg->Op, this->option_strip_obj_ns, nsp_name, obj_name);
            hash_name_rgb(nsp_name, nsp_hash_clr);
            hash_name_rgb(obj_name, obj_hash_clr);
            if (cachable) {
                // only values that will be valid for the whole node, sg->Op, are cachable.
                // the source of manually overriden values is not known and may therefore not be cached. 
                CRYPTOMATTE_CACHE[sg->tid].object = sg->Op;
                CRYPTOMATTE_CACHE[sg->tid].obj_hash_clr = *obj_hash_clr;
                CRYPTOMATTE_CACHE[sg->tid].nsp_hash_clr = *nsp_hash_clr;
            }
        }

        AtNode *shader = AiShaderGlobalsGetShader(sg);
        if (CRYPTOMATTE_CACHE[sg->tid].shader_object == sg->Op) {
            *mat_hash_clr = CRYPTOMATTE_CACHE[sg->tid].mat_hash_clr;
        } else {
            AtArray *shaders = AiNodeGetArray(sg->Op, aStr_shader);
            bool cachable = shaders ? AiArrayGetNumElements(shaders) == 1 : false;

            char mat_name[MAX_STRING_LENGTH] = "";
            cachable = get_material_name(sg, sg->Op, shader, this->option_strip_mat_ns, mat_name) && cachable;
            hash_name_rgb(mat_name, mat_hash_clr);

            if (cachable) {
                // only values that will be valid for the whole node, sg->Op, are cachable.
                CRYPTOMATTE_CACHE[sg->tid].shader_object = sg->Op;
                CRYPTOMATTE_CACHE[sg->tid].mat_hash_clr = *mat_hash_clr;
            }
        }
    }

    ///////////////////////////////////////////////
    //      Building Cryptomatte Arnold Nodes
    ///////////////////////////////////////////////

    void setup_cryptomatte_nodes() {
        AtNode * renderOptions = AiUniverseGetOptions();
        const AtArray * outputs = AiNodeGetArray( renderOptions, "outputs");

        AtArray *tmp_uc_drivers = NULL, *uc_aov_array = NULL, *uc_src_array = NULL;

        const int prev_output_num = AiArrayGetNumElements(outputs);
        int num_cryptomatte_AOVs = 0;
        if (this->user_cryptomatte_info != NULL) {
            num_cryptomatte_AOVs += AiArrayGetNumElements(this->user_cryptomatte_info) - 2;
            uc_aov_array = AiArrayGetArray(this->user_cryptomatte_info, 0);
            uc_src_array = AiArrayGetArray(this->user_cryptomatte_info, 1);
            tmp_uc_drivers = AiArrayAllocate(AiArrayGetNumElements(uc_aov_array), 1, AI_TYPE_NODE); // destroyed later
            for (uint32_t i=0; i<AiArrayGetNumElements(uc_aov_array); i++ )
                AiArraySetPtr(tmp_uc_drivers, i, NULL);
        }

        AtNode *driver_cryptoAsset = NULL, *driver_cryptoObject = NULL, *driver_cryptoMaterial = NULL;
        num_cryptomatte_AOVs += 3;

        AtArray *tmp_new_outputs = AiArrayAllocate(num_cryptomatte_AOVs * this->option_aov_depth, 1, AI_TYPE_STRING); // destroyed later
        int new_output_num = 0;

        for (uint32_t i=0; i < prev_output_num; i++) {
            size_t output_string_chars = AiArrayGetStr( outputs, i).length();
            char temp_string[MAX_STRING_LENGTH * 8]; 
            memset(temp_string, 0, sizeof(temp_string));
            strncpy(temp_string, AiArrayGetStr( outputs, i), output_string_chars);

            char *c0 = strtok(temp_string," ");
            char *c1 = strtok(NULL," ");
            char *c2 = strtok(NULL," ");
            char *c3 = strtok(NULL," ");
            char *c4 = strtok(NULL," ");   
            
            bool short_output = (c4 == NULL);
            char * aov_name =      short_output ? c0 : c1;
            char * aov_type_name = short_output ? c1 : c2;
            char * filter_name =   short_output ? c2 : c3;
            char * driver_name =   short_output ? c3 : c4;

            AtNode * driver = NULL;
            AtArray * cryptoAOVs = NULL;
            if (strcmp( aov_name, this->aov_cryptoasset.c_str()) == 0) {
                this->aovArray_cryptoasset = AiArrayAllocate(this->option_aov_depth, 1, AI_TYPE_STRING);
                cryptoAOVs = this->aovArray_cryptoasset;
                driver = AiNodeLookUpByName(driver_name);
                driver_cryptoAsset = driver;
            } else if (strcmp( aov_name, this->aov_cryptoobject.c_str()) == 0) {
                this->aovArray_cryptoobject = AiArrayAllocate(this->option_aov_depth, 1, AI_TYPE_STRING);
                cryptoAOVs = this->aovArray_cryptoobject;
                driver = AiNodeLookUpByName(driver_name);
                driver_cryptoObject = driver;
            } else if (strcmp( aov_name, this->aov_cryptomaterial.c_str()) == 0) {
                this->aovArray_cryptomaterial = AiArrayAllocate(this->option_aov_depth, 1, AI_TYPE_STRING);
                cryptoAOVs = this->aovArray_cryptomaterial;
                driver = AiNodeLookUpByName(driver_name);
                driver_cryptoMaterial = driver;
            } else if (this->user_cryptomatte_info) {
                for (uint32_t j=0; j < AiArrayGetNumElements(uc_aov_array); j++) {
                    const char * user_aov_name = AiArrayGetStr(uc_aov_array, j);
                    if (strcmp(aov_name, user_aov_name) == 0) {
                        cryptoAOVs = AiArrayAllocate(this->option_aov_depth, 1, AI_TYPE_STRING); // will be destroyed when cryptomatteData is
                        driver = AiNodeLookUpByName(driver_name);
                        AiArraySetPtr(tmp_uc_drivers, j, driver);
                        AiArraySetArray(this->user_cryptomatte_info, j + 2, cryptoAOVs);
                        break;
                    }
                }
            }

            if (cryptoAOVs != NULL)
                new_output_num += this->create_AOV_array(driver, aov_name, filter_name, cryptoAOVs, 
                                                         tmp_new_outputs, new_output_num);
        }

        if (new_output_num > 0) {
            const int total_outputs = prev_output_num + new_output_num;
            AtArray * final_outputs = AiArrayAllocate(total_outputs, 1, AI_TYPE_STRING); // Does not need destruction
            for (uint32_t i=0; i < prev_output_num; i++) {
                // Iterate through old outputs and add them
                AiArraySetStr(final_outputs, i, AiArrayGetStr(outputs, i));
            }
            for (int i=0; i < new_output_num; i++)  {
                // Iterate through new outputs and add them
                AiArraySetStr(final_outputs, i + prev_output_num, AiArrayGetStr(tmp_new_outputs, i));
            }
            AiNodeSetArray(renderOptions, "outputs", final_outputs );
        }

        build_standard_metadata(driver_cryptoAsset, driver_cryptoObject, driver_cryptoMaterial,
                                this->aov_cryptoasset, this->aov_cryptoobject, this->aov_cryptomaterial, 
                                this->option_strip_obj_ns, this->option_strip_mat_ns);
        build_user_metadata(this->user_cryptomatte_info, tmp_uc_drivers);

        AiArrayDestroy(tmp_new_outputs);
        if (tmp_uc_drivers)
            AiArrayDestroy(tmp_uc_drivers);
    }

    int create_AOV_array(AtNode *driver, const char *aov_name, const char *filter_name, 
                          AtArray *cryptoAOVs, AtArray *tmp_new_outputs, int output_offset) 
    {
        // helper for setup_cryptomatte_nodes. Populates cryptoAOVs and returns the number of new outputs created. 
        int new_ouputs = 0;

        if (!AiNodeIs(driver, AtString("driver_exr"))) {
            AiMsgWarning("Cryptomatte Error: Can only write Cryptomatte to EXR files.");
            return new_ouputs;
        }

        ///////////////////////////////////////////////
        //      Compile info about original filter 

        float aFilter_width = 2.0;
        char aFilter_filter[128];
        AtNode * orig_filter = AiNodeLookUpByName(filter_name);
        const AtNodeEntry * orig_filter_nodeEntry = AiNodeGetNodeEntry(orig_filter);
        const char * orig_filter_type_name = AiNodeEntryGetName(orig_filter_nodeEntry);
        if (AiNodeEntryLookUpParameter(orig_filter_nodeEntry, "width") != NULL) {
            aFilter_width = AiNodeGetFlt(orig_filter, "width");             
        }

        memset(aFilter_filter, 0, sizeof(aFilter_filter));
        size_t filter_name_len = strlen(orig_filter_type_name);
        strncpy(aFilter_filter, orig_filter_type_name, filter_name_len);            
        char *filter_strip_point = strstr(aFilter_filter, "_filter");
        if (filter_strip_point != NULL) {
            filter_strip_point[0] = '\0';
        }

        ///////////////////////////////////////////////
        //      Set CryptoAOV driver to full precision and outlaw RLE

        AiNodeSetBool(driver, "half_precision", false);

        const AtNodeEntry* driver_entry = AiNodeGetNodeEntry(driver);
        AtEnum compressions =  AiParamGetEnum(AiNodeEntryLookUpParameter(driver_entry, "compression"));         
        if (AiNodeGetInt(driver, "compression") == AiEnumGetValue(compressions, "rle")) {
            AiMsgWarning("Cryptomatte cannot be set to RLE compression- it does not work on full float. Switching to Zip.");
            AiNodeSetStr(driver, "compression", "zip");
        }
        
        AiAOVRegister(aov_name, AI_TYPE_RGB, AI_AOV_BLEND_OPACITY);

        AtArray *outputs = AiNodeGetArray( AiUniverseGetOptions(), "outputs");
        
        std::unordered_set<std::string> outputSet;
        for (int i=0; i < AiArrayGetNumElements(outputs); i++)
            outputSet.insert( std::string(AiArrayGetStr(outputs, i)));

        ///////////////////////////////////////////////
        //      Create filters and outputs as needed 
        for (int i=0; i<this->option_aov_depth; i++) {
            char filter_rank_name[MAX_STRING_LENGTH];
            memset(filter_rank_name, 0, MAX_STRING_LENGTH);

            char aov_rank_name[MAX_STRING_LENGTH];
            memset(aov_rank_name, 0, MAX_STRING_LENGTH);

            size_t aov_name_chars = strlen(aov_name);
            strncpy(filter_rank_name, aov_name, aov_name_chars);
            strncpy(aov_rank_name, aov_name, aov_name_chars);

            char rank_number_string[MAX_STRING_LENGTH];
            memset(rank_number_string, 0, MAX_STRING_LENGTH);
            sprintf(rank_number_string, "%002d", i);

            strcat(filter_rank_name, "_filter" );
            strcat(filter_rank_name, rank_number_string );
            strcat(aov_rank_name, rank_number_string );
            
            const bool nofilter = AiNodeLookUpByName( filter_rank_name ) == NULL;
            if ( nofilter ) {
                AtNode *filter = AiNode("cryptomatte_filter");
                AiNodeSetStr(filter, "name", filter_rank_name);
                AiNodeSetInt(filter, "rank", i*2);
                AiNodeSetStr(filter, "filter", aFilter_filter);
                AiNodeSetFlt(filter, "width", aFilter_width);
                AiNodeSetStr(filter, "mode", "double_rgba");
            }

            std::string new_output_str(aov_rank_name);
            new_output_str += " " ;
            new_output_str += "FLOAT" ;
            new_output_str += " " ;
            new_output_str += filter_rank_name ;
            new_output_str += " " ;
            new_output_str += AiNodeGetName(driver);

            if (outputSet.find(new_output_str) == outputSet.end()) {
                AiAOVRegister(aov_rank_name, AI_TYPE_FLOAT, AI_AOV_BLEND_NONE);
                AiArraySetStr(tmp_new_outputs, output_offset + new_ouputs, new_output_str.c_str());
                new_ouputs++;
            }
            AiArraySetStr(cryptoAOVs, i, aov_rank_name);
        }
        return new_ouputs;
    }

    ///////////////////////////////////////////////
    //      Cleanup
    ///////////////////////////////////////////////

    void destroy_arrays() {
        if (this->aovArray_cryptoasset)
            AiArrayDestroy(this->aovArray_cryptoasset);
        if (this->aovArray_cryptoobject)
            AiArrayDestroy(this->aovArray_cryptoobject);
        if (this->aovArray_cryptomaterial)
            AiArrayDestroy(this->aovArray_cryptomaterial);
        if (this->user_cryptomatte_info) {
            for (uint32_t i=0; i<AiArrayGetNumElements(this->user_cryptomatte_info); i++) {
                AtArray *subarray = AiArrayGetArray(this->user_cryptomatte_info, i);
                AiArraySetArray(this->user_cryptomatte_info, i, NULL);
                if (subarray)
                    AiArrayDestroy(subarray);   
            }
            AiArrayDestroy(this->user_cryptomatte_info);
        }
        this->aovArray_cryptoasset = NULL;
        this->aovArray_cryptoobject = NULL;
        this->aovArray_cryptomaterial = NULL;
        this->user_cryptomatte_info = NULL;
    }

public:
    ~CryptomatteData() {
        this->destroy_arrays();
    }
};


