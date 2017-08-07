#include <string>
#include <unordered_set>
#include <cstring>
#include <map>
#include <vector>
#include <ctime>
#include "MurmurHash3.h"
#include <cstdio>
#include <limits>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <ai.h>

#include "cryptomatte.h"

#define NOMINMAX // lets you keep using std::min

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

// Internal
#define CRYPTOMATTE_METADATA_SET_FLAG "already_has_crypto_metadata_"

// Some static AtStrings to cache
const static AtString aStr_shader("shader");

unsigned char g_pointcloud_instance_verbosity = 0;  // to do: remove this.


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

    // C4DtoA: c4d|obj_hierarchy|...
    if (strstr(nsp_name, "c4d|") == nsp_name) {
        // Chop first element
        char *obj_name_start = nsp_name + 4;
        memmove(obj_name_out, obj_name_start, strlen(obj_name_start));
        // Snip second element
        char *nsp_name_start = strtok(obj_name_start, "|");
        if (nsp_name_start != NULL) {
            strcpy(nsp_name_out, nsp_name_start);
        }
        return;
    }

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

    // C4DtoA: c4d|mat_name|root_node_name
    if (strstr(mat_name_out, "c4d|") == mat_name_out) {
        // Chop first element
        char *str_cut = mat_name_out + 4;
        // Snip second element
        char *mat_name_start = strtok(str_cut, "|");
        if (mat_name_start != NULL) {
            memmove(mat_name_out, mat_name_start, strlen(mat_name_start) + 1);
        }
        return;
    }

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


void hash_name_rgb(const char *name, AtRGB *out_color) {
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

bool get_object_names(const AtShaderGlobals *sg, const AtNode *node, bool strip_obj_ns, 
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


bool get_material_name(const AtShaderGlobals *sg, const AtNode *node, const AtNode *shader, bool strip_mat_ns, 
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


void write_array_of_AOVs(AtShaderGlobals *sg, const AtArray *names, float id) {
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

typedef std::vector<std::string> string_vector_t;
typedef std::vector<string_vector_t> string_vector_vector_t;

void compute_metadata_ID(char id_buffer[8], AtString cryptomatte_name) {
    AtRGB color_hash;
    hash_name_rgb(cryptomatte_name.c_str(), &color_hash);
    uint32_t float_bits;
    std::memcpy(&float_bits, &color_hash.r, 4);
    char hex_chars[9];
    sprintf(hex_chars, "%08x", float_bits);
    strncpy(id_buffer, hex_chars, 7);
    id_buffer[7] = '\0';
}

void write_manifest_to_string(manf_map_t *map, std::string &manf_string) {
    manf_map_it map_it = map->begin();
    const size_t map_entries = map->size();
    const size_t max_entries = 100000;
    size_t metadata_entries = map_entries;
    if (map_entries > max_entries) {
        AiMsgWarning("Cryptomatte: %lu entries in manifest, limiting to %lu", map_entries, max_entries);
        metadata_entries = max_entries;
    }

    manf_string.append("{");
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
        manf_string.append(pair);
    }
    manf_string.append("}");
}

void write_manifest_sidecar_file(manf_map_t *map_md_asset, string_vector_t manifest_paths) {
    std::string encoded_manifest = "";
    write_manifest_to_string(map_md_asset, encoded_manifest);
    for (size_t i=0; i<manifest_paths.size(); i++) {
        std::ofstream out(manifest_paths[i].c_str());
        AiMsgInfo("[Cryptomatte] writing file, %s", manifest_paths[i].c_str());
        out << encoded_manifest.c_str();
        out.close();
    }
}

bool check_driver(AtNode *driver) {
    return driver != NULL && AiNodeIs(driver, AtString("driver_exr"));
}

void write_metadata_to_driver(AtNode * driver, AtString cryptomatte_name, manf_map_t *map, std::string sidecar_manif_file) {
    if (!check_driver(driver))
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
    if (sidecar_manif_file.length()) {
        metadata_manf = prefix + std::string("manif_file ") + sidecar_manif_file;
    } else {
        metadata_manf = prefix + std::string("manifest ");
        write_manifest_to_string(map, metadata_manf);
    }
    
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
    return check_driver(driver) && !AiNodeLookUpUserParameter(driver, flag.c_str());
}


void metadata_set_unneeded(AtNode* driver, const AtString aov_name) {
    if (driver == NULL)
        return;
    std::string flag = std::string(CRYPTOMATTE_METADATA_SET_FLAG) + aov_name.c_str();
    if (AiNodeLookUpUserParameter(driver, flag.c_str()) == NULL)
        AiNodeDeclare(driver, flag.c_str(), "constant BOOL");
}


void add_hash_to_map(const char *c_str, manf_map_t *md_map) {
    if (!string_has_content(c_str))
        return;
    AtRGB hash;
    std::string name_string = std::string(c_str);
    if (md_map->count(name_string) == 0) {
        hash_name_rgb(c_str, &hash);
        (*md_map)[name_string] = hash.r;
    }
}


void add_obj_to_manifest(const AtNode *node, char name[MAX_STRING_LENGTH], 
                         const AtString offset_user_data, manf_map_t *hash_map ) {
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


struct UserCryptomattes {
    size_t count;
    std::vector<AtArray*> aov_arrays;
    std::vector<AtString> aovs;
    std::vector<AtString> sources;

    UserCryptomattes() {
        this->clear();
    }

    UserCryptomattes(const AtArray *aov_input, const AtArray *src_input) {
        this->clear();
        if (aov_input == NULL || src_input == NULL)
            return;

        const uint32_t num_inputs = std::min(AiArrayGetNumElements(aov_input), 
                                             AiArrayGetNumElements(src_input));

        for (uint32_t i=0; i<num_inputs; i++) {
            const AtString aov = AiArrayGetStr(aov_input, i);
            const AtString src = AiArrayGetStr(src_input, i);
            if (!aov.empty() && !src.empty()) {
                AiMsgInfo("Adding user-Cryptomatte %lu: AOV: %s Source user data: %s",
                          this->aovs.size(), aov.c_str(), src.c_str());
                this->aovs.push_back(aov);
                this->sources.push_back(src);
            }
        }
        this->aov_arrays.resize(this->aovs.size());
        for (int i=0; i<this->aovs.size(); i++)
            this->aov_arrays[i] = NULL;

        this->count = this->aovs.size();
    }

    ~UserCryptomattes() {
        for (size_t i=0; i<this->aov_arrays.size(); i++) {
            if (this->aov_arrays[i])
                AiArrayDestroy(this->aov_arrays[i]); 
        }
    }

private:
    void clear() {
        // helper. Does not deallocate aov arrays. 
        this->count = 0;
        this->aov_arrays = std::vector<AtArray*>();
        this->aovs = std::vector<AtString>();
        this->sources = std::vector<AtString>();
    }
};



///////////////////////////////////////////////
//
//      Shader Data Struct
//
///////////////////////////////////////////////

struct CryptomatteData {
    // Accessed during sampling, so hopefully in first cache line. 
    AtString aov_cryptoasset;
    AtString aov_cryptoobject;
    AtString aov_cryptomaterial;

    AtArray * aov_array_cryptoasset;
    AtArray * aov_array_cryptoobject;
    AtArray * aov_array_cryptomaterial;
    UserCryptomattes user_cryptomattes;

    // User options. 
    uint8_t option_depth;
    uint8_t option_aov_depth;
    bool    option_strip_obj_ns;
    bool    option_strip_mat_ns;
    uint8_t option_pcloud_ice_verbosity;
    bool    option_sidecar_manifests;

    // Vector of paths for each of the cryptomattes. Vector because each cryptomatte can write
    // to multiple drivers (stereo, multi-camera)
    string_vector_t manif_asset_paths;
    string_vector_t manif_object_paths;
    string_vector_t manif_material_paths;
    // Nested vector of paths for each user cryptomatte. 
    string_vector_vector_t manifs_user_paths;

public:
    CryptomatteData() {
        this->aov_array_cryptoasset    = NULL;
        this->aov_array_cryptoobject   = NULL;
        this->aov_array_cryptomaterial = NULL;
        this->user_cryptomattes = UserCryptomattes();
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

        this->user_cryptomattes = UserCryptomattes(uc_aov_array, uc_src_array);
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

    void set_option_sidecar_manifests(bool sidecar) {
        this->option_sidecar_manifests = sidecar;
    }

    void do_cryptomattes(AtShaderGlobals *sg ) {
        if (sg->Rt & AI_RAY_CAMERA && sg->sc == AI_CONTEXT_SURFACE) {
            this->do_standard_cryptomattes(sg);
            this->do_user_cryptomattes(sg);
        }
    }

    void write_sidecar_manifests() {
        this->write_standard_sidecar_manifests();
        this->write_user_sidecar_manifests();
    }

private:
    void do_standard_cryptomattes(AtShaderGlobals *sg) {
        if (!this->aov_array_cryptoasset && !this->aov_array_cryptoobject && !this->aov_array_cryptomaterial)
            return;
        
        AtRGB nsp_hash_clr, obj_hash_clr, mat_hash_clr;
        this->hash_object_rgb(sg, &nsp_hash_clr, &obj_hash_clr, &mat_hash_clr);

        if (this->aov_array_cryptoasset)
            write_array_of_AOVs(sg, this->aov_array_cryptoasset, nsp_hash_clr.r);
        if (this->aov_array_cryptoobject)
            write_array_of_AOVs(sg, this->aov_array_cryptoobject, obj_hash_clr.r);
        if (this->aov_array_cryptomaterial)
            write_array_of_AOVs(sg, this->aov_array_cryptomaterial, mat_hash_clr.r);
        
        nsp_hash_clr.r = obj_hash_clr.r = mat_hash_clr.r = 0.0f;

        AiAOVSetRGBA(sg, this->aov_cryptoasset, nsp_hash_clr);      
        AiAOVSetRGBA(sg, this->aov_cryptoobject, obj_hash_clr);
        AiAOVSetRGBA(sg, this->aov_cryptomaterial, mat_hash_clr);
    }

    void do_user_cryptomattes(AtShaderGlobals * sg ) {
        for (uint32_t i=0; i<this->user_cryptomattes.count; i++) {
            AtArray * aovArray = this->user_cryptomattes.aov_arrays[i];
            if (aovArray != NULL) {
                AtString aov_name = this->user_cryptomattes.aovs[i];
                AtString src_data_name = this->user_cryptomattes.sources[i];
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

    void hash_object_rgb(AtShaderGlobals* sg, AtRGB *nsp_hash_clr, AtRGB *obj_hash_clr, AtRGB *mat_hash_clr) {
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
        AtNode *renderOptions = AiUniverseGetOptions();
        const AtArray * outputs = AiNodeGetArray(renderOptions, "outputs");

        std::vector<std::vector<AtNode *>> tmp_uc_drivers_vv;
        tmp_uc_drivers_vv.resize(this->user_cryptomattes.count);

        const uint32_t prev_output_num = AiArrayGetNumElements(outputs);

        std::vector<AtNode*> driver_cryptoAsset_v, driver_cryptoObject_v, driver_cryptoMaterial_v;
        string_vector_t new_outputs;

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
            char * camera_name =   short_output ? NULL : c0;
            char * aov_name =      short_output ? c0 : c1;
            char * filter_name =   short_output ? c2 : c3;
            char * driver_name =   short_output ? c3 : c4;

            AtNode * driver = NULL;
            AtArray * cryptoAOVs = NULL;

            if (strcmp( aov_name, this->aov_cryptoasset.c_str()) == 0) {
                this->aov_array_cryptoasset = AiArrayAllocate(this->option_aov_depth, 1, AI_TYPE_STRING);
                cryptoAOVs = this->aov_array_cryptoasset;
                driver = AiNodeLookUpByName(driver_name);
                driver_cryptoAsset_v.push_back(driver);
            } else if (strcmp( aov_name, this->aov_cryptoobject.c_str()) == 0) {
                this->aov_array_cryptoobject = AiArrayAllocate(this->option_aov_depth, 1, AI_TYPE_STRING);
                cryptoAOVs = this->aov_array_cryptoobject;
                driver = AiNodeLookUpByName(driver_name);
                driver_cryptoObject_v.push_back(driver);
            } else if (strcmp( aov_name, this->aov_cryptomaterial.c_str()) == 0) {
                this->aov_array_cryptomaterial = AiArrayAllocate(this->option_aov_depth, 1, AI_TYPE_STRING);
                cryptoAOVs = this->aov_array_cryptomaterial;
                driver = AiNodeLookUpByName(driver_name);
                driver_cryptoMaterial_v.push_back(driver);
            } else if (this->user_cryptomattes.count != 0) {
                for (size_t j=0; j < this->user_cryptomattes.count; j++) {
                    const char * user_aov_name = this->user_cryptomattes.aovs[j].c_str();
                    if (strcmp(aov_name, user_aov_name) == 0) {
                        cryptoAOVs = AiArrayAllocate(this->option_aov_depth, 1, AI_TYPE_STRING); // will be destroyed when cryptomatteData is
                        driver = AiNodeLookUpByName(driver_name);
                        tmp_uc_drivers_vv[j].push_back(driver);
                        this->user_cryptomattes.aov_arrays[j] = cryptoAOVs;
                        break;
                    }
                }
            }

            if (cryptoAOVs != NULL) {
                for (uint32_t j=0; j<this->option_aov_depth; j++)
                    AiArraySetStr(cryptoAOVs, j, "");
                this->create_AOV_array(aov_name, filter_name, camera_name, driver, cryptoAOVs, &new_outputs);
            }
        }

        if (new_outputs.size() > 0) {
            if (this->option_sidecar_manifests) {
                AtString manifest_driver_name("cryptomatte_manifest_driver");
                AtNode *manifest_driver = AiNodeLookUpByName(manifest_driver_name);
                if (manifest_driver == NULL) {
                    manifest_driver = AiNode("cryptomatte_manifest_driver");
                    AiNodeSetStr(manifest_driver, "name", manifest_driver_name);
                }
                AiNodeSetLocalData(manifest_driver, this);
                new_outputs.push_back(manifest_driver_name.c_str());
            }
            const uint32_t total_outputs = prev_output_num + (uint32_t) new_outputs.size();
            AtArray * final_outputs = AiArrayAllocate(total_outputs, 1, AI_TYPE_STRING); // Does not need destruction
            for (uint32_t i=0; i < prev_output_num; i++) {
                // Iterate through old outputs and add them
                AiArraySetStr(final_outputs, i, AiArrayGetStr(outputs, i));
            }
            for (int i=0; i < new_outputs.size(); i++)  {
                // Iterate through new outputs and add them
                AiArraySetStr(final_outputs, i + prev_output_num, new_outputs[i].c_str());
            }
            AiNodeSetArray(renderOptions, "outputs", final_outputs );
        }

        this->build_standard_metadata(driver_cryptoAsset_v, driver_cryptoObject_v, driver_cryptoMaterial_v);
        this->build_user_metadata(tmp_uc_drivers_vv);
    }

    void setup_deferred_manifest(AtNode *driver, AtString token, std::string &path_out, std::string &metadata_path_out) {
        path_out = "";
        metadata_path_out = "";
        if (check_driver(driver) && this->option_sidecar_manifests) {                
            std::string filepath = std::string(AiNodeGetStr(driver, "filename").c_str());
            const size_t exr_found = filepath.find(".exr");
            if (exr_found != std::string::npos)
                filepath = filepath.substr(0, exr_found);
            
            path_out = filepath + "." + token.c_str() + ".json";
            const size_t last_partition = path_out.find_last_of("/\\");
            if (last_partition == std::string::npos)
                metadata_path_out += path_out;
            else
                metadata_path_out += path_out.substr(last_partition + 1);
        }
    }

    void write_standard_sidecar_manifests() {
        const bool do_md_asset = this->manif_asset_paths.size() > 0;
        const bool do_md_object = this->manif_object_paths.size() > 0;
        const bool do_md_material = this->manif_material_paths.size() > 0;

        if (!do_md_asset && !do_md_object && !do_md_material)
            return;

        manf_map_t map_md_asset, map_md_object, map_md_material;
        this->compile_standard_manifests(do_md_asset, do_md_object, do_md_material, map_md_asset, map_md_object, map_md_material);
        
        if (do_md_asset)
            write_manifest_sidecar_file(&map_md_asset, this->manif_asset_paths);
        if (do_md_object)
            write_manifest_sidecar_file(&map_md_object, this->manif_object_paths);
        if (do_md_material)
            write_manifest_sidecar_file(&map_md_material, this->manif_material_paths);

        // reset sidecar writers
        this->manif_asset_paths = string_vector_t();
        this->manif_object_paths = string_vector_t();
        this->manif_material_paths = string_vector_t();
    }

    void compile_standard_manifests(bool do_md_asset, bool do_md_object, bool do_md_material, 
                                    manf_map_t &map_md_asset, manf_map_t &map_md_object, manf_map_t &map_md_material) {
        AtNodeIterator * shape_iterator = AiUniverseGetNodeIterator(AI_NODE_SHAPE);
        while (!AiNodeIteratorFinished(shape_iterator)) {
            AtNode *node = AiNodeIteratorGetNext(shape_iterator);
            if (!node)
                continue;
            char nsp_name[MAX_STRING_LENGTH] = "";
            char obj_name[MAX_STRING_LENGTH] = "";

            get_object_names(NULL, node, this->option_strip_obj_ns, nsp_name, obj_name);

            if (do_md_asset || do_md_object) { 
                add_obj_to_manifest(node, nsp_name, CRYPTO_ASSET_OFFSET_UDATA, &map_md_asset);
                add_obj_to_manifest(node, obj_name, CRYPTO_OBJECT_OFFSET_UDATA, &map_md_object);
            }
            if (do_md_material) {
                // Process all shaders from the objects into the manifest. 
                // This includes cluster materials.
                AtArray *shaders = AiNodeGetArray(node, "shader");
                if (!shaders)
                    continue;
                for (uint32_t i = 0; i < AiArrayGetNumElements(shaders); i++) {
                    char mat_name[MAX_STRING_LENGTH] = "";
                    AtNode *shader = static_cast<AtNode*>(AiArrayGetPtr(shaders, i));
                    if (!shader)
                        continue;
                    get_material_name(NULL, node, shader, this->option_strip_mat_ns, mat_name);
                    add_obj_to_manifest(node, mat_name, CRYPTO_MATERIAL_OFFSET_UDATA, &map_md_material);
                }
            }
        }
        AiNodeIteratorDestroy(shape_iterator);
    }

    void write_user_sidecar_manifests() {
        std::vector<bool> do_metadata;
        do_metadata.resize(this->user_cryptomattes.count);
        std::vector<manf_map_t> manf_maps;
        manf_maps.resize(this->user_cryptomattes.count);

        for (size_t i=0; i < this->user_cryptomattes.count; i++) {
            do_metadata[i] = true;
            for (size_t j=0; j < this->manifs_user_paths[i].size(); j++)
                do_metadata[i] = do_metadata[i] && this->manifs_user_paths[i][j].length() > 0;
        }
        this->compile_user_manifests(do_metadata, manf_maps);

        for (size_t i=0; i < this->manifs_user_paths.size(); i++)
            if (do_metadata[i])
                write_manifest_sidecar_file(&manf_maps[i], this->manifs_user_paths[i]);

        this->manifs_user_paths = string_vector_vector_t();
    }

    void compile_user_manifests(std::vector<bool> &do_metadata, std::vector<manf_map_t> manf_maps) {
        if (this->user_cryptomattes.count == 0)
            return;
        AtNodeIterator * shape_iterator = AiUniverseGetNodeIterator(AI_NODE_SHAPE);
        while (!AiNodeIteratorFinished(shape_iterator)) {
            AtNode *node = AiNodeIteratorGetNext(shape_iterator);
            for (uint32_t i=0; i<this->user_cryptomattes.count; i++) {
                if (!do_metadata[i])
                    continue;
                AtString user_data_name = this->user_cryptomattes.sources[i]; // should really be atstring
                const AtUserParamEntry* pentry = AiNodeLookUpUserParameter(node, user_data_name);
                if (pentry == NULL|| AiUserParamGetType(pentry) != AI_TYPE_STRING)
                    continue;

                if ( AiUserParamGetCategory(pentry) == AI_USERDEF_CONSTANT) {
                    // not an array
                    add_hash_to_map(AiNodeGetStr(node, user_data_name), &manf_maps[i]);
                } else {
                    AtArray * values = AiNodeGetArray(node, user_data_name);
                    if (values != NULL) {
                        for (uint32_t ai=0; ai<AiArrayGetNumElements(values); ai++)
                            add_hash_to_map(AiArrayGetStr(values, ai), &manf_maps[i]);
                    }
                }
            }
        }
        AiNodeIteratorDestroy(shape_iterator);
    }

    void build_standard_metadata(std::vector<AtNode*> driver_asset_v, std::vector<AtNode*> driver_object_v, 
                                 std::vector<AtNode*> driver_material_v) {
        const clock_t metadata_start_time = clock();

        bool do_md_asset = true, do_md_object = true, do_md_material = true;
        for (size_t i = 0; i<driver_asset_v.size(); i++) {
            do_md_asset = do_md_asset && metadata_needed(driver_asset_v[i], this->aov_cryptoasset);
            metadata_set_unneeded(driver_asset_v[i], this->aov_cryptoasset);
        }
        for (size_t i = 0; i<driver_object_v.size(); i++) {
            do_md_object = do_md_object && metadata_needed(driver_object_v[i], this->aov_cryptoobject);
            metadata_set_unneeded(driver_object_v[i], this->aov_cryptoobject);
        }
        for (size_t i = 0; i<driver_material_v.size(); i++) {
            do_md_material = do_md_material && metadata_needed(driver_material_v[i], this->aov_cryptomaterial);
            metadata_set_unneeded(driver_material_v[i], this->aov_cryptomaterial);
        }

        if (!do_md_asset && !do_md_object && !do_md_material)
            return;

        manf_map_t map_md_asset, map_md_object, map_md_material;

        if (!this->option_sidecar_manifests)
            this->compile_standard_manifests(do_md_asset, do_md_object, do_md_material, map_md_asset, map_md_object, map_md_material);

        std::string manif_asset_m, manif_object_m, manif_material_m;
        this->manif_asset_paths.resize(driver_asset_v.size());
        for (size_t i = 0; i<driver_asset_v.size(); i++) {
            this->setup_deferred_manifest(driver_asset_v[i], this->aov_cryptoasset, this->manif_asset_paths[i], manif_asset_m);
            write_metadata_to_driver(driver_asset_v[i], this->aov_cryptoasset, &map_md_asset, manif_asset_m);
        }
        this->manif_object_paths.resize(driver_object_v.size());
        for (size_t i = 0; i<driver_object_v.size(); i++) {
            this->setup_deferred_manifest(driver_object_v[i], this->aov_cryptoobject, this->manif_object_paths[i], manif_object_m);
            write_metadata_to_driver(driver_object_v[i], this->aov_cryptoobject, &map_md_object, manif_object_m);
        }
        this->manif_material_paths.resize(driver_material_v.size());
        for (size_t i = 0; i<driver_material_v.size(); i++) {
            this->setup_deferred_manifest(driver_material_v[i], this->aov_cryptomaterial, this->manif_material_paths[i], manif_material_m);
            write_metadata_to_driver(driver_material_v[i], this->aov_cryptomaterial, &map_md_material, manif_material_m);
        }

        if (!this->option_sidecar_manifests)
            AiMsgInfo("Cryptomatte manifest created - %f seconds", (float( clock () - metadata_start_time ) /  CLOCKS_PER_SEC));
        else
            AiMsgInfo("Cryptomatte manifest creation deferred - sidecar file written at end of render.");
    }

    void build_user_metadata(std::vector<std::vector<AtNode *>> drivers_vv) {
        string_vector_vector_t manifs_user_m;
        this->manifs_user_paths = string_vector_vector_t();
        manifs_user_m.resize(drivers_vv.size());
        this->manifs_user_paths.resize(drivers_vv.size());

        const bool sidecar = this->option_sidecar_manifests;
        if (this->user_cryptomattes.count == 0 || drivers_vv.size() == 0)
            return;


        const clock_t metadata_start_time = clock();
        std::vector<bool> do_metadata;
        do_metadata.resize(this->user_cryptomattes.count);
        std::vector<manf_map_t> manf_maps;
        manf_maps.resize(this->user_cryptomattes.count);

        bool do_anything = false;
        for (uint32_t i=0; i<drivers_vv.size(); i++) {
            do_metadata[i] = false;
            for (size_t j=0; j<drivers_vv[i].size(); j++) {
                AtNode *driver = drivers_vv[i][j];
                AtString user_aov = this->user_cryptomattes.aovs[i];
                do_metadata[i] = do_metadata[i] || metadata_needed(driver, user_aov); // checks for null
                do_anything = do_anything || do_metadata[i];

                std::string manif_user_m;
                if (sidecar) {
                    std::string manif_asset_paths;
                    this->setup_deferred_manifest(driver, user_aov, manif_asset_paths, manif_user_m);
                    this->manifs_user_paths[i].push_back( driver == NULL ? ""  : manif_asset_paths);
                }
                manifs_user_m[i].push_back( driver == NULL ? ""  : manif_user_m);
            }
        }

        if (!do_anything)
            return;

        if (!sidecar)
            this->compile_user_manifests(do_metadata, manf_maps);

        for (uint32_t i=0; i<drivers_vv.size(); i++) {
            if (!do_metadata[i])
                continue;
            AtString aov_name = this->user_cryptomattes.aovs[i];
            for (size_t j=0; j<drivers_vv[i].size(); j++) {
                AtNode *driver = drivers_vv[i][j];
                if (driver) {
                    metadata_set_unneeded(driver, aov_name);
                    write_metadata_to_driver(driver, aov_name, &manf_maps[i], manifs_user_m[i][j]);
                }
            }
        }
        AiMsgInfo("User Cryptomatte manifests created - %f seconds",
                  float(clock() - metadata_start_time) / CLOCKS_PER_SEC);
    }

    void create_AOV_array(const char *aov_name, const char *filter_name, const char *camera_name, 
                          AtNode *driver, AtArray *cryptoAOVs, string_vector_t *new_ouputs) {
        // helper for setup_cryptomatte_nodes. Populates cryptoAOVs and returns the number of new outputs created. 
        if (!check_driver(driver)) {
            AiMsgWarning("Cryptomatte Error: Can only write Cryptomatte to EXR files.");
            return;
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
        for (uint32_t i=0; i < AiArrayGetNumElements(outputs); i++)
            outputSet.insert( std::string(AiArrayGetStr(outputs, i)));

        std::unordered_set<std::string> splitAOVs;
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

            std::string new_output_str;
            if (camera_name != NULL)
                new_output_str += std::string(camera_name) + " ";
            new_output_str += aov_rank_name;
            new_output_str += " " ;
            new_output_str += "FLOAT" ;
            new_output_str += " " ;
            new_output_str += filter_rank_name ;
            new_output_str += " " ;
            new_output_str += AiNodeGetName(driver);

            if (outputSet.find(new_output_str) == outputSet.end()) {
                AiAOVRegister(aov_rank_name, AI_TYPE_FLOAT, AI_AOV_BLEND_NONE);
                new_ouputs->push_back(new_output_str.c_str());
            }
            AiArraySetStr(cryptoAOVs, i, aov_rank_name);
        }
    }

    ///////////////////////////////////////////////
    //      Cleanup
    ///////////////////////////////////////////////

    void destroy_arrays() {
        if (this->aov_array_cryptoasset)
            AiArrayDestroy(this->aov_array_cryptoasset);
        if (this->aov_array_cryptoobject)
            AiArrayDestroy(this->aov_array_cryptoobject);
        if (this->aov_array_cryptomaterial)
            AiArrayDestroy(this->aov_array_cryptomaterial);
        this->aov_array_cryptoasset = NULL;
        this->aov_array_cryptoobject = NULL;
        this->aov_array_cryptomaterial = NULL;
        this->user_cryptomattes = UserCryptomattes();
    }

public:
    ~CryptomatteData() {
        this->destroy_arrays();
    }
};



///////////////////////////////////////////////
//
//      Called in shaders/drivers
//
///////////////////////////////////////////////

CryptomatteData* CryptomatteData_new() {
    return new CryptomatteData;
}
void CryptomatteData_setup_all(CryptomatteData *data, const AtString aov_cryptoasset, 
                               const AtString aov_cryptoobject, const AtString aov_cryptomaterial,
                               AtArray *uc_aov_array, AtArray *uc_src_array) {
    data->setup_all(aov_cryptoasset, aov_cryptoobject, aov_cryptomaterial, uc_aov_array, uc_src_array);
}
void CryptomatteData_set_option_depth(CryptomatteData *data, int depth) {
    data->set_option_depth(depth);
}
void CryptomatteData_set_option_namespace_stripping(CryptomatteData *data, bool strip_obj, bool strip_mat) {
    data->set_option_namespace_stripping(strip_obj, strip_mat);
}
void CryptomatteData_set_option_ice_pcloud_verbosity(CryptomatteData *data, int verbosity) {
    data->set_option_ice_pcloud_verbosity(verbosity);
}
void CryptomatteData_set_option_sidecar_manifests(CryptomatteData *data, bool sidecar) {
    data->set_option_sidecar_manifests(sidecar);
}
void CryptomatteData_do_cryptomattes(CryptomatteData *data, AtShaderGlobals *sg ) {
    data->do_cryptomattes(sg);
}
void CryptomatteData_write_sidecar_manifests(CryptomatteData *data) {
    data->write_sidecar_manifests();
}
void CryptomatteData_del(CryptomatteData *data) {
    delete data;
}
