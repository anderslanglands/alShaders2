#include <ai.h>

/*
API Documentation:

This no longer needs to be built into every shader as in Arnold 4, thanks to AOV
shaders. If an alternate AOV shader
is made, it should should still interact with cryptomatte entirely through the
CryptomatteData struct.
How to add cryptomatte to a shader:

1. Add a member of type *CryptomatteData to your ShaderData

    node_initialize {
        CryptomatteData* data = new CryptomatteData();
        AiNodeSetLocalData(node, data);
    }

    The constructor sets up getting the global state of things, including
getting global
    options declared on the Arnold options.

2. in node_finish:

    node_finish {
        CryptomatteData* data =
            reinterpret_cast<CryptomatteData*>(AiNodeGetLocalData(node));
        delete data;
    }

3. in node_update:

    node_update {
        CryptomatteData* data =
            reinterpret_cast<CryptomatteData*>(AiNodeGetLocalData(node));

        // set cryptomatte depth (optional)
        data->set_option_channels(AiNodeGetInt(node, "cryptomatte_depth"),
                                CRYPTO_PREVIEWINEXR_DEFAULT);

        // set namespace options (optional)
        data->set_option_namespace_stripping(
            AiNodeGetBool(node, "strip_obj_namespaces"),
            AiNodeGetBool(node, "strip_mat_namespaces"));

        // set option for sidecar manifest (optional)
        data->set_manifest_sidecar(sidecar);

        AtArray* uc_aov_array = AiArray(
            4, 1, AI_TYPE_STRING, AiNodeGetStr(node, "user_crypto_aov_0").c_str(),
            AiNodeGetStr(node, "user_crypto_aov_1").c_str(),
            AiNodeGetStr(node, "user_crypto_aov_2").c_str(),
            AiNodeGetStr(node, "user_crypto_aov_3").c_str());
        AtArray* uc_src_array = AiArray(
            4, 1, AI_TYPE_STRING, AiNodeGetStr(node, "user_crypto_src_0").c_str(),
            AiNodeGetStr(node, "user_crypto_src_1").c_str(),
            AiNodeGetStr(node, "user_crypto_src_2").c_str(),
            AiNodeGetStr(node, "user_crypto_src_3").c_str());

        // does all the setup work. User cryptomatte arrays are optional (can be
        // nulls).
        // The three arguments are the names of the cryptomatte AOVs. If the
        // AOVs are active (connected to EXR drivers), this does all the
        // complicated setup work of creating multiple AOVs if necessary,
        // writing metadata, etc.
        data->setup_all(AiNodeGetStr(node, "aov_crypto_asset"),
                        AiNodeGetStr(node, "aov_crypto_object"),
                        AiNodeGetStr(node, "aov_crypto_material"), uc_aov_array,
                        uc_src_array);
    }


4. in shader_evaluate:

    shader_evaluate {
        if (sg->Rt & AI_RAY_CAMERA && sg->sc == AI_CONTEXT_SURFACE) {
            CryptomatteData* data =
                reinterpret_cast<CryptomatteData*>(AiNodeGetLocalData(node));
            // This does all the hashing and writing values to AOVs, including
            // user-defined cryptomattes. Opacity does not need to be supplied.
            data->do_cryptomattes(sg);
        }
    }


*/

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include "MurmurHash3.h"
#define NOMINMAX // lets you keep using std::min on windows

using ManifestMap = std::map<std::string, float>;

using StringVector = std::vector<std::string>;

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

// System values
#define MAX_STRING_LENGTH 255
#define MAX_CRYPTOMATTE_DEPTH 99
#define MAX_USER_CRYPTOMATTES 16

extern uint8_t g_pointcloud_instance_verbosity;

// Internal
#define CRYPTOMATTE_METADATA_SET_FLAG "already_has_crypto_metadata_"

// User data names
extern const AtString CRYPTO_ASSET_UDATA;
extern const AtString CRYPTO_OBJECT_UDATA;
extern const AtString CRYPTO_MATERIAL_UDATA;

extern const AtString CRYPTO_ASSET_OFFSET_UDATA;
extern const AtString CRYPTO_OBJECT_OFFSET_UDATA;
extern const AtString CRYPTO_MATERIAL_OFFSET_UDATA;

// Some static AtStrings to cache
const AtString aStr_shader("shader");
const AtString aStr_list_aggregate("list_aggregate");

///////////////////////////////////////////////
//
//      String processing
//
///////////////////////////////////////////////

inline void safe_copy_to_buffer(char buffer[MAX_STRING_LENGTH], const char* c) {
    if (c != NULL) {
        size_t length = std::min(strlen(c), (size_t)MAX_STRING_LENGTH - 1);
        strncpy(buffer, c, length);
    } else {
        buffer[0] = '\0';
    }
}

inline bool string_has_content(const char* c) { return c != NULL && c[0] != '\0'; }

///////////////////////////////////////////////
//
//      Name processing
//
///////////////////////////////////////////////

inline bool sitoa_pointcloud_instance_handling(const char* obj_full_name,
                                               char obj_name_out[MAX_STRING_LENGTH]) {
    if (g_pointcloud_instance_verbosity == 0 || strstr(obj_full_name, ".SItoA.Instance.") == NULL) {
        return false;
    }
    char obj_name[MAX_STRING_LENGTH];
    safe_copy_to_buffer(obj_name, obj_full_name);

    char* instance_start = strstr(obj_name, ".SItoA.Instance.");
    if (!instance_start)
        return false;

    char* space = strstr(instance_start, " ");
    if (!space)
        return false;

    char* instance_name = &space[1];
    char* obj_suffix2 = strstr(instance_name, ".SItoA.");
    if (!obj_suffix2)
        return false;
    obj_suffix2[0] = '\0'; // strip the suffix
    size_t chars_to_copy = strlen(instance_name);
    if (chars_to_copy >= MAX_STRING_LENGTH || chars_to_copy == 0) {
        return false;
    }
    if (g_pointcloud_instance_verbosity == 2) {
        char* frame_numbers = &instance_start[16]; // 16 chars in ".SItoA.Instance.", this gets us
                                                   // to the first number
        char* instance_ID = strstr(frame_numbers, ".");
        if (!instance_ID)
            return false;
        char* instance_ID_end = strstr(instance_ID, " ");
        if (!instance_ID_end)
            return false;
        instance_ID_end[0] = '\0';
        size_t ID_len = strlen(instance_ID);
        strncpy(&instance_name[chars_to_copy], instance_ID, ID_len);
        chars_to_copy += ID_len;
    }

    strncpy(obj_name_out, instance_name, chars_to_copy);
    return true;
}

inline void mtoa_strip_namespaces(const char* obj_full_name, char obj_name_out[MAX_STRING_LENGTH]) {
    char* to = obj_name_out;
    size_t len = 0;
    size_t sublen = 0;
    const char* from = obj_full_name;
    const char* end = from + strlen(obj_full_name);
    const char* found = strchr(from, '|');
    const char* sep = NULL;

    while (found != NULL) {
        sep = strchr(from, ':');
        if (sep != NULL && sep < found) {
            from = sep + 1;
        }
        sublen = found - from;
        memmove(to, from, sublen);
        to[sublen] = '|';

        len += sublen + 1;
        to += sublen + 1;
        from = found + 1;

        found = strchr(from, '|');
    }

    sep = strchr(from, ':');
    if (sep != NULL && sep < end) {
        from = sep + 1;
    }
    sublen = end - from;
    memmove(to, from, sublen);
    to[sublen] = '\0';
}

inline void get_clean_object_name(const char* obj_full_name, char obj_name_out[MAX_STRING_LENGTH],
                                  char nsp_name_out[MAX_STRING_LENGTH], bool strip_obj_ns) {
    char nsp_name[MAX_STRING_LENGTH] = "";
    safe_copy_to_buffer(nsp_name, obj_full_name);
    bool obj_already_done = false;

    const uint8_t mode_maya = 0;
    const uint8_t mode_c4d = 1;
    const uint8_t mode_si = 2;
    uint8_t mode = mode_maya;

    // C4DtoA: c4d|obj_hierarchy|...
    if (strncmp(nsp_name, "c4d|", 4) == 0) {
        mode = mode_c4d;
        const char* nsp = nsp_name + 4;
        size_t len = strlen(nsp);
        memmove(nsp_name, nsp, len);
        nsp_name[len] = '\0';
    }

    char* sitoa_suffix = strstr(nsp_name, ".SItoA.");
    if (sitoa_suffix) { // in Softimage mode
        mode = mode_si;
        obj_already_done = sitoa_pointcloud_instance_handling(obj_full_name, obj_name_out);
        sitoa_suffix[0] = '\0'; // cut off everything after the start of .SItoA
    }

    char* nsp_separator = NULL;
    if (mode == mode_c4d) // find last c4d mode switch {
        nsp_separator = strrchr(nsp_name, '|');
    else if (mode == mode_si)
        nsp_separator = strchr(nsp_name, '.');
    else if (mode == mode_maya)
        nsp_separator = strchr(nsp_name, ':');

    if (!obj_already_done) {
        if (!nsp_separator || !strip_obj_ns) { // use whole name
            memmove(obj_name_out, nsp_name, strlen(nsp_name));
        } else if (mode == mode_maya) { // maya
            mtoa_strip_namespaces(nsp_name, obj_name_out);
        } else { // take everything right of sep
            char* obj_name_start = nsp_separator + 1;
            memmove(obj_name_out, obj_name_start, strlen(obj_name_start));
        }
    }

    if (nsp_separator) {
        nsp_separator[0] = '\0';
        strcpy(nsp_name_out, nsp_name); // copy namespace
    } else {
        strncpy(nsp_name_out, "default\0", 8); // default namespace
    }
}

inline void get_clean_material_name(const char* mat_full_name, char mat_name_out[MAX_STRING_LENGTH],
                                    bool strip_ns) {
    safe_copy_to_buffer(mat_name_out, mat_full_name);

    // C4DtoA: c4d|mat_name|root_node_name
    if (strncmp(mat_name_out, "c4d|", 4) == 0) {
        // Chop first element
        char* str_cut = mat_name_out + 4;
        // Snip second element
        char* mat_name_start = strtok(str_cut, "|");
        if (mat_name_start != NULL) {
            memmove(mat_name_out, mat_name_start, strlen(mat_name_start) + 1);
        }
        return;
    }

    // Example:
    //      Softimage:
    //      Sources.Materials.myLibrary_ref_library.myMaterialName.Standard_Mattes.uBasic.SITOA.25000....
    //      Maya: namespace:my_material_sg
    char* mat_postfix = strstr(mat_name_out, ".SItoA.");
    if (mat_postfix != NULL) {
        //   Sources.Materials.myLibrary_ref_library.myMaterialName.Standard_Mattes.uBasic
        //   <<chop>> .SITOA.25000....
        mat_postfix[0] = '\0';

        char* mat_shader_name = strrchr(mat_name_out, '.');
        if (mat_shader_name != NULL) {
            //   Sources.Materials.myLibrary_ref_library.myMaterialName.Standard_Mattes
            //   <<chop>> .uBasic
            mat_shader_name[0] = '\0';
        }

        char* Standard_Mattes = strstr(mat_name_out, ".Standard_Mattes");
        if (Standard_Mattes != NULL)
            //   Sources.Materials.myLibrary_ref_library.myMaterialName <<chop>>
            //   .Standard_Mattes
            Standard_Mattes[0] = '\0';

        const char* prefix = "Sources.Materials.";
        const char* mat_prefix_seperator = strstr(mat_name_out, prefix);
        if (mat_prefix_seperator != NULL) {
            //   Sources.Materials. <<SNIP>>
            //   myLibrary_ref_library.myMaterialName
            const char* mat_name_start = mat_prefix_seperator + strlen(prefix);
            memmove(mat_name_out, mat_name_start, strlen(mat_name_start) + 1);
        }

        char* nsp_separator = strchr(mat_name_out, '.');
        if (strip_ns && nsp_separator != NULL) {
            //   myLibrary_ref_library. <<SNIP>> myMaterialName
            nsp_separator[0] = '\0';
            char* mat_name_start = nsp_separator + 1;
            memmove(mat_name_out, mat_name_start, strlen(mat_name_start) + 1);
        }
        // Leaving us with just the material name.
    }

    // For maya, you get something simpler, like namespace:my_material_sg.
    char* ns_separator = strchr(mat_name_out, ':');
    if (strip_ns && ns_separator != NULL) {
        //    namespace: <<SNIP>> my_material_sg
        ns_separator[0] = '\0';
        char* mat_name_start = ns_separator + 1;
        memmove(mat_name_out, mat_name_start, strlen(mat_name_start) + 1);
    }
}

inline float hash_to_float(uint32_t hash) {
    // if all exponent bits are 0 (subnormals, +zero, -zero) set exponent to 1
    // if all exponent bits are 1 (NaNs, +inf, -inf) set exponent to 254
    uint32_t exponent = hash >> 23 & 255; // extract exponent (8 bits)
    if (exponent == 0 || exponent == 255)
        hash ^= 1 << 23; // toggle bit
    float f;
    std::memcpy(&f, &hash, 4);
    return f;
}

inline AtRGB hash_name_rgb(const char* name) {
    // This puts the float ID into the red channel, and the human-readable
    // versions into the G and B channels.
    uint32_t m3hash = 0;
    AtRGB out_color;
    MurmurHash3_x86_32(name, (uint32_t)strlen(name), 0, &m3hash);
    out_color.r = hash_to_float(m3hash);
    out_color.g = ((float)((m3hash << 8)) / (float)std::numeric_limits<uint32_t>::max());
    out_color.b = ((float)((m3hash << 16)) / (float)std::numeric_limits<uint32_t>::max());
    return out_color;
}

inline AtString get_user_data(const AtShaderGlobals* sg, const AtNode* node,
                              const AtString user_data_name, bool* cachable) {
    // returns the string if the parameter is usable, modifies cachable
    const AtUserParamEntry* pentry = AiNodeLookUpUserParameter(node, user_data_name);
    if (pentry) {
        if (AiUserParamGetType(pentry) == AI_TYPE_STRING &&
            AiUserParamGetCategory(pentry) == AI_USERDEF_CONSTANT) {
            return AiNodeGetStr(node, user_data_name);
        }
    }
    if (sg) {
        // this is intentionally outside the if (pentry) block.
        // With user data declared on ginstances and such, no pentry
        // is aquirable but AiUDataGetStr still works.
        // still true in Arnold 5.
        AtString udata_value;
        if (AiUDataGetStr(user_data_name, udata_value)) {
            *cachable = false;
            return udata_value;
        }
    }
    return AtString();
}

inline int get_offset_user_data(const AtShaderGlobals* sg, const AtNode* node,
                                const AtString user_data_name, bool* cachable) {
    // returns the string if the parameter is usable, modifies cachable
    const AtUserParamEntry* pentry = AiNodeLookUpUserParameter(node, user_data_name);
    if (pentry) {
        if (AiUserParamGetType(pentry) == AI_TYPE_INT &&
            AiUserParamGetCategory(pentry) == AI_USERDEF_CONSTANT) {
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

inline void offset_name(const AtShaderGlobals* sg, const AtNode* node, const int offset,
                        char obj_name_out[MAX_STRING_LENGTH]) {
    if (offset) {
        char offset_num_str[12];
        sprintf(offset_num_str, "_%d", offset);
        strcat(obj_name_out, offset_num_str);
    }
}

inline bool get_object_names(const AtShaderGlobals* sg, const AtNode* node, bool strip_obj_ns,
                             char nsp_name_out[MAX_STRING_LENGTH],
                             char obj_name_out[MAX_STRING_LENGTH]) {
    bool cachable = true;

    const AtString nsp_user_data = get_user_data(sg, node, CRYPTO_ASSET_UDATA, &cachable);
    const AtString obj_user_data = get_user_data(sg, node, CRYPTO_OBJECT_UDATA, &cachable);

    bool need_nsp_name = nsp_user_data.empty();
    bool need_obj_name = obj_user_data.empty();
    if (need_obj_name || need_nsp_name) {
        const char* obj_full_name = AiNodeGetName(node);
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

    nsp_name_out[MAX_STRING_LENGTH - 1] = '\0';
    return cachable;
}

inline bool get_material_name(const AtShaderGlobals* sg, const AtNode* node, const AtNode* shader,
                              bool strip_mat_ns, char mat_name_out[MAX_STRING_LENGTH]) {
    bool cachable = true;
    AtString mat_user_data = get_user_data(sg, node, CRYPTO_MATERIAL_UDATA, &cachable);

    get_clean_material_name(AiNodeGetName(shader), mat_name_out, strip_mat_ns);
    offset_name(sg, node, get_offset_user_data(sg, node, CRYPTO_MATERIAL_OFFSET_UDATA, &cachable),
                mat_name_out);

    if (!mat_user_data.empty())
        strcpy(mat_name_out, mat_user_data);

    mat_name_out[MAX_STRING_LENGTH - 1] = '\0';
    return cachable;
}

///////////////////////////////////////////////
//
//      Metadata Writing
//
///////////////////////////////////////////////

inline void compute_metadata_ID(char id_buffer[8], AtString cryptomatte_name) {
    AtRGB color_hash = hash_name_rgb(cryptomatte_name.c_str());
    uint32_t float_bits;
    std::memcpy(&float_bits, &color_hash.r, 4);
    char hex_chars[9];
    sprintf(hex_chars, "%08x", float_bits);
    strncpy(id_buffer, hex_chars, 7);
    id_buffer[7] = '\0';
}

inline void write_manifest_to_string(const ManifestMap& map, std::string& manf_string) {
    ManifestMap::const_iterator map_it = map.begin();
    const size_t map_entries = map.size();
    const size_t max_entries = 100000;
    size_t metadata_entries = map_entries;
    if (map_entries > max_entries) {
        AiMsgWarning("Cryptomatte: %lu entries in manifest, limiting to %lu", //
                     map_entries, max_entries);
        metadata_entries = max_entries;
    }

    manf_string.append("{");
    std::string pair;
    pair.reserve(MAX_STRING_LENGTH);
    for (uint32_t i = 0; i < metadata_entries; i++) {
        std::string name = map_it->first;
        float hash_value = map_it->second;
        ++map_it;

        uint32_t float_bits;
        std::memcpy(&float_bits, &hash_value, 4);
        char hex_chars[9];
        sprintf(hex_chars, "%08x", float_bits);

        pair.clear();
        pair.append("\"");
        for (size_t j = 0; j < name.length(); j++) {
            // append the name, char by char
            const char c = name.at(j);
            if (c == '"' || c == '\\' || c == '/')
                pair += "\\";
            pair += c;
        }
        pair.append("\":\"");
        pair.append(hex_chars);
        pair.append("\"");
        if (i < map_entries - 1)
            pair.append(",");
        manf_string.append(pair);
    }
    manf_string.append("}");
}

inline void write_manifest_sidecar_file(const ManifestMap& map_md_asset,
                                        StringVector manifest_paths) {
    std::string encoded_manifest = "";
    write_manifest_to_string(map_md_asset, encoded_manifest);
    for (const auto& manifest_path : manifest_paths) {
        std::ofstream out(manifest_path.c_str());
        AiMsgInfo("[Cryptomatte] writing file, %s", manifest_path.c_str());
        out << encoded_manifest.c_str();
        out.close();
    }
}

inline bool check_driver(AtNode* driver) {
    return driver != NULL && AiNodeIs(driver, AtString("driver_exr"));
}

inline void write_metadata_to_driver(AtNode* driver, AtString cryptomatte_name,
                                     const ManifestMap& map, std::string sidecar_manif_file) {
    if (!check_driver(driver))
        return;

    AtArray* orig_md = AiNodeGetArray(driver, "custom_attributes");
    const uint32_t orig_num_entries = orig_md ? AiArrayGetNumElements(orig_md) : 0;

    std::string metadata_hash, metadata_conv, metadata_name,
        metadata_manf; // the new entries
    AtArray* combined_md =
        AiArrayAllocate(orig_num_entries + 4, 1, AI_TYPE_STRING); // Does not need destruction

    std::string prefix("STRING cryptomatte/");
    char metadata_id_buffer[8];
    compute_metadata_ID(metadata_id_buffer, cryptomatte_name);
    prefix += std::string(metadata_id_buffer) + std::string("/");

    for (uint32_t i = 0; i < orig_num_entries; i++) {
        if (prefix.compare(AiArrayGetStr(orig_md, i)) == 0) {
            AiMsgWarning("Cryptomatte: Unable to write metadata. EXR metadata "
                         "key, %s, already in use.",
                         prefix.c_str());
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

    for (uint32_t i = 0; i < orig_num_entries; i++) {
        AiArraySetStr(combined_md, i, AiArrayGetStr(orig_md, i));
    }
    AiArraySetStr(combined_md, orig_num_entries + 0, metadata_manf.c_str());
    AiArraySetStr(combined_md, orig_num_entries + 1, metadata_hash.c_str());
    AiArraySetStr(combined_md, orig_num_entries + 2, metadata_conv.c_str());
    AiArraySetStr(combined_md, orig_num_entries + 3, metadata_name.c_str());

    AiNodeSetArray(driver, "custom_attributes", combined_md);
}

inline bool metadata_needed(AtNode* driver, const AtString aov_name) {
    std::string flag = std::string(CRYPTOMATTE_METADATA_SET_FLAG) + aov_name.c_str();
    return check_driver(driver) && !AiNodeLookUpUserParameter(driver, flag.c_str());
}

inline void metadata_set_unneeded(AtNode* driver, const AtString aov_name) {
    if (driver == NULL)
        return;
    std::string flag = std::string(CRYPTOMATTE_METADATA_SET_FLAG) + aov_name.c_str();
    if (AiNodeLookUpUserParameter(driver, flag.c_str()) == NULL)
        AiNodeDeclare(driver, flag.c_str(), "constant BOOL");
}

inline void add_hash_to_map(const char* c_str, ManifestMap& md_map) {
    if (!string_has_content(c_str))
        return;
    std::string name_string = std::string(c_str);
    if (md_map.count(name_string) == 0) {
        AtRGB hash = hash_name_rgb(c_str);
        md_map[name_string] = hash.r;
    }
}

inline AtString add_override_udata_to_manifest(const AtNode* node, const AtString override_udata,
                                               ManifestMap& hash_map) {
    /*
    Adds override user data to manifest map, including arrays of overrides.
    Does not add offsets.
    */
    const AtUserParamEntry* pentry = AiNodeLookUpUserParameter(node, override_udata);
    if (pentry == NULL || AiUserParamGetType(pentry) != AI_TYPE_STRING)
        return AtString();

    if (AiUserParamGetCategory(pentry) == AI_USERDEF_CONSTANT) {
        // not an array
        AtString udata = AiNodeGetStr(node, override_udata);
        add_hash_to_map(udata, hash_map);
        return udata;
    } else {
        AtArray* values = AiNodeGetArray(node, override_udata);
        if (values) {
            for (uint32_t ai = 0; ai < AiArrayGetNumElements(values); ai++)
                add_hash_to_map(AiArrayGetStr(values, ai), hash_map);
        }
        return AtString();
    }
}

inline void add_obj_to_manifest(const AtNode* node, char name[MAX_STRING_LENGTH],
                                AtString override_udata, const AtString offset_udata,
                                ManifestMap& hash_map) {
    /*
    Adds objects to the manifest, based on processed names and potentially user
    data overrides
    and offsets.

    Does not handle combining overrides and offsets, unless the overrides are
    already passed
    in as "name", as is the case with single value per node overrides.
    */
    add_hash_to_map(name, hash_map);
    add_override_udata_to_manifest(node, override_udata, hash_map);
    bool single_offset_val = true;
    get_offset_user_data(NULL, node, offset_udata, &single_offset_val);
    if (!single_offset_val) { // means offset was an array
        AtArray* offsets = AiNodeGetArray(node, offset_udata);
        if (offsets) {
            std::unordered_set<int> visitedOffsets;
            for (uint32_t i = 0; i < AiArrayGetNumElements(offsets); i++) {
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

///////////////////////////////////////////////
//
//      AOV utilities
//
///////////////////////////////////////////////

inline void write_array_of_AOVs(AtShaderGlobals* sg, const AtArray* names, float id) {
    for (uint32_t i = 0; i < AiArrayGetNumElements(names); i++) {
        AtString aovName = AiArrayGetStr(names, i);
        if (aovName.empty())
            return;
        AiAOVSetFlt(sg, aovName, id);
    }
}

///////////////////////////////////////////////
//
//      CryptomatteCache
//
///////////////////////////////////////////////

#define CACHE_LINE 64
#if defined(_WIN32) || defined(_MSC_VER)
#define CACHE_ALIGN __declspec(align(CACHE_LINE))
#else
#define CACHE_ALIGN __attribute__((aligned(CACHE_LINE)))
#endif

struct CACHE_ALIGN CryptomatteCache {
    AtNode* object = nullptr;
    AtRGB nsp_hash_clr = AI_RGB_BLACK;
    AtRGB obj_hash_clr = AI_RGB_BLACK;
    AtNode* shader_object = nullptr;
    AtRGB mat_hash_clr = AI_RGB_BLACK;
};

extern CryptomatteCache CRYPTOMATTE_CACHE[AI_MAX_THREADS];


///////////////////////////////////////////////
//
//      UserCryptomatte and CryptomatteData 
//
///////////////////////////////////////////////

struct UserCryptomattes {
    size_t count = 0;
    std::vector<AtArray*> aov_arrays;
    std::vector<AtString> aovs;
    std::vector<AtString> sources;

    UserCryptomattes() {}

    UserCryptomattes(const AtArray* aov_input, const AtArray* src_input) {
        if (aov_input == NULL || src_input == NULL)
            return;

        const uint32_t num_inputs =
            std::min(AiArrayGetNumElements(aov_input), AiArrayGetNumElements(src_input));

        for (uint32_t i = 0; i < num_inputs; i++) {
            const AtString aov = AiArrayGetStr(aov_input, i);
            const AtString src = AiArrayGetStr(src_input, i);
            if (!aov.empty() && !src.empty()) {
                AiMsgInfo("Adding user-Cryptomatte %lu: AOV: %s Source user data: %s", aovs.size(),
                          aov.c_str(), src.c_str());
                aovs.push_back(aov);
                sources.push_back(src);
            }
        }
        aov_arrays.resize(aovs.size());
        for (auto& aov_array : aov_arrays)
            aov_array = nullptr;

        count = aovs.size();
    }

    ~UserCryptomattes() {
        for (auto& aov_array : aov_arrays) {
            AiArrayDestroy(aov_array);
        }
    }
};

struct CryptomatteData {
    // Accessed during sampling, so hopefully in first cache line.
    AtString aov_cryptoasset;
    AtString aov_cryptoobject;
    AtString aov_cryptomaterial;

    AtArray* aov_array_cryptoasset = nullptr;
    AtArray* aov_array_cryptoobject = nullptr;
    AtArray* aov_array_cryptomaterial = nullptr;
    UserCryptomattes user_cryptomattes;

    // User options.
    uint8_t option_depth;
    uint8_t option_aov_depth;
    bool option_preview_channels;
    bool option_strip_obj_ns;
    bool option_strip_mat_ns;
    uint8_t option_pcloud_ice_verbosity;
    bool option_sidecar_manifests;

    // Vector of paths for each of the cryptomattes. Vector because each
    // cryptomatte can write to multiple drivers (stereo, multi-camera)
    StringVector manif_asset_paths;
    StringVector manif_object_paths;
    StringVector manif_material_paths;
    // Nested vector of paths for each user cryptomatte.
    std::vector<StringVector> manifs_user_paths;

public:
    CryptomatteData() {
        set_option_channels(CRYPTO_DEPTH_DEFAULT, CRYPTO_PREVIEWINEXR_DEFAULT);
        set_option_namespace_stripping(CRYPTO_STRIPOBJNS_DEFAULT, CRYPTO_STRIPMATNS_DEFAULT);
        set_option_ice_pcloud_verbosity(CRYPTO_ICEPCLOUDVERB_DEFAULT);
    }

    void setup_all(const AtString aov_cryptoasset_, const AtString aov_cryptoobject_,
                   const AtString aov_cryptomaterial_, AtArray* uc_aov_array,
                   AtArray* uc_src_array) {
        aov_cryptoasset = aov_cryptoasset_;
        aov_cryptoobject = aov_cryptoobject_;
        aov_cryptomaterial = aov_cryptomaterial_;

        destroy_arrays();

        user_cryptomattes = UserCryptomattes(uc_aov_array, uc_src_array);
        setup_cryptomatte_nodes();
    }

    void set_option_channels(int depth, bool previewchannels) {
        depth = std::min(std::max(depth, 1), MAX_CRYPTOMATTE_DEPTH);
        option_depth = depth;
        option_preview_channels = previewchannels;
        if (option_depth % 2 == 0)
            option_aov_depth = option_depth / 2;
        else
            option_aov_depth = (option_depth + 1) / 2;
    }

    void set_option_namespace_stripping(bool strip_obj, bool strip_mat) {
        option_strip_obj_ns = strip_obj;
        option_strip_mat_ns = strip_mat;
    }

    void set_option_ice_pcloud_verbosity(int verbosity) {
        verbosity = std::min(std::max(verbosity, 0), 2);
        option_pcloud_ice_verbosity = verbosity;
        g_pointcloud_instance_verbosity = option_pcloud_ice_verbosity; // to do: really remove this
    }

    void set_option_sidecar_manifests(bool sidecar) { option_sidecar_manifests = sidecar; }

    void do_cryptomattes(AtShaderGlobals* sg) {
        if (sg->Rt & AI_RAY_CAMERA && sg->sc == AI_CONTEXT_SURFACE) {
            do_standard_cryptomattes(sg);
            do_user_cryptomattes(sg);
        }
    }

    void write_sidecar_manifests() {
        write_standard_sidecar_manifests();
        write_user_sidecar_manifests();
    }

private:
    void do_standard_cryptomattes(AtShaderGlobals* sg) {
        if (!aov_array_cryptoasset && !aov_array_cryptoobject && !aov_array_cryptomaterial)
            return;

        AtRGB nsp_hash_clr, obj_hash_clr, mat_hash_clr;
        hash_object_rgb(sg, nsp_hash_clr, obj_hash_clr, mat_hash_clr);

        if (aov_array_cryptoasset)
            write_array_of_AOVs(sg, aov_array_cryptoasset, nsp_hash_clr.r);
        if (aov_array_cryptoobject)
            write_array_of_AOVs(sg, aov_array_cryptoobject, obj_hash_clr.r);
        if (aov_array_cryptomaterial)
            write_array_of_AOVs(sg, aov_array_cryptomaterial, mat_hash_clr.r);

        if (option_preview_channels) {
            nsp_hash_clr.r = obj_hash_clr.r = mat_hash_clr.r = 0.0f;

            AiAOVSetRGBA(sg, aov_cryptoasset, nsp_hash_clr);
            AiAOVSetRGBA(sg, aov_cryptoobject, obj_hash_clr);
            AiAOVSetRGBA(sg, aov_cryptomaterial, mat_hash_clr);
        }
    }

    void do_user_cryptomattes(AtShaderGlobals* sg) {
        for (uint32_t i = 0; i < user_cryptomattes.count; i++) {
            AtArray* aovArray = user_cryptomattes.aov_arrays[i];
            if (aovArray != NULL) {
                AtString aov_name = user_cryptomattes.aovs[i];
                AtString src_data_name = user_cryptomattes.sources[i];
                AtRGB hash = AI_RGB_BLACK;
                AtString result;

                AiUDataGetStr(src_data_name, result);
                if (!result.empty())
                    hash = hash_name_rgb(result.c_str());

                write_array_of_AOVs(sg, aovArray, hash.r);
                if (option_preview_channels) {
                    hash.r = 0.0f;
                    AiAOVSetRGBA(sg, aov_name, hash);
                }
            }
        }
    }

    void hash_object_rgb(AtShaderGlobals* sg, AtRGB& nsp_hash_clr, AtRGB& obj_hash_clr,
                         AtRGB& mat_hash_clr) {
        if (CRYPTOMATTE_CACHE[sg->tid].object == sg->Op) {
            nsp_hash_clr = CRYPTOMATTE_CACHE[sg->tid].nsp_hash_clr;
            obj_hash_clr = CRYPTOMATTE_CACHE[sg->tid].obj_hash_clr;
        } else {
            char nsp_name[MAX_STRING_LENGTH] = "";
            char obj_name[MAX_STRING_LENGTH] = "";
            bool cachable = get_object_names(sg, sg->Op, option_strip_obj_ns, nsp_name, obj_name);
            nsp_hash_clr = hash_name_rgb(nsp_name);
            obj_hash_clr = hash_name_rgb(obj_name);
            if (cachable) {
                // only values that will be valid for the whole node, sg->Op,
                // are cachable.
                // the source of manually overriden values is not known and may
                // therefore not be cached.
                CRYPTOMATTE_CACHE[sg->tid].object = sg->Op;
                CRYPTOMATTE_CACHE[sg->tid].obj_hash_clr = obj_hash_clr;
                CRYPTOMATTE_CACHE[sg->tid].nsp_hash_clr = nsp_hash_clr;
            }
        }

        AtNode* shader = AiShaderGlobalsGetShader(sg);
        if (CRYPTOMATTE_CACHE[sg->tid].shader_object == sg->Op) {
            mat_hash_clr = CRYPTOMATTE_CACHE[sg->tid].mat_hash_clr;
        } else {
            AtArray* shaders = AiNodeGetArray(sg->Op, aStr_shader);
            bool cachable = shaders ? AiArrayGetNumElements(shaders) == 1 : false;

            char mat_name[MAX_STRING_LENGTH] = "";
            cachable =
                get_material_name(sg, sg->Op, shader, option_strip_mat_ns, mat_name) && cachable;
            mat_hash_clr = hash_name_rgb(mat_name);

            if (cachable) {
                // only values that will be valid for the whole node, sg->Op,
                // are cachable.
                CRYPTOMATTE_CACHE[sg->tid].shader_object = sg->Op;
                CRYPTOMATTE_CACHE[sg->tid].mat_hash_clr = mat_hash_clr;
            }
        }
    }

    ///////////////////////////////////////////////
    //      Building Cryptomatte Arnold Nodes
    ///////////////////////////////////////////////

    void setup_cryptomatte_nodes() {
        AtNode* renderOptions = AiUniverseGetOptions();
        const AtArray* outputs = AiNodeGetArray(renderOptions, "outputs");

        std::vector<std::vector<AtNode*>> tmp_uc_drivers_vv;
        tmp_uc_drivers_vv.resize(user_cryptomattes.count);

        const uint32_t prev_output_num = AiArrayGetNumElements(outputs);

        std::vector<AtNode*> driver_cryptoAsset_v, driver_cryptoObject_v, driver_cryptoMaterial_v;
        StringVector new_outputs;

        for (uint32_t i = 0; i < prev_output_num; i++) {
            size_t output_string_chars = AiArrayGetStr(outputs, i).length();
            char temp_string[MAX_STRING_LENGTH * 8];
            memset(temp_string, 0, sizeof(temp_string));
            strncpy(temp_string, AiArrayGetStr(outputs, i), output_string_chars);

            char* c0 = strtok(temp_string, " ");
            char* c1 = strtok(NULL, " ");
            char* c2 = strtok(NULL, " ");
            char* c3 = strtok(NULL, " ");
            char* c4 = strtok(NULL, " ");

            bool short_output = (c4 == NULL);
            char* camera_name = short_output ? NULL : c0;
            char* aov_name = short_output ? c0 : c1;
            char* filter_name = short_output ? c2 : c3;
            char* driver_name = short_output ? c3 : c4;

            AtNode* driver = NULL;
            AtArray* cryptoAOVs = NULL;

            if (strcmp(aov_name, aov_cryptoasset.c_str()) == 0) {
                aov_array_cryptoasset = AiArrayAllocate(option_aov_depth, 1, AI_TYPE_STRING);
                cryptoAOVs = aov_array_cryptoasset;
                driver = AiNodeLookUpByName(driver_name);
                driver_cryptoAsset_v.push_back(driver);
            } else if (strcmp(aov_name, aov_cryptoobject.c_str()) == 0) {
                aov_array_cryptoobject = AiArrayAllocate(option_aov_depth, 1, AI_TYPE_STRING);
                cryptoAOVs = aov_array_cryptoobject;
                driver = AiNodeLookUpByName(driver_name);
                driver_cryptoObject_v.push_back(driver);
            } else if (strcmp(aov_name, aov_cryptomaterial.c_str()) == 0) {
                aov_array_cryptomaterial = AiArrayAllocate(option_aov_depth, 1, AI_TYPE_STRING);
                cryptoAOVs = aov_array_cryptomaterial;
                driver = AiNodeLookUpByName(driver_name);
                driver_cryptoMaterial_v.push_back(driver);
            } else if (user_cryptomattes.count != 0) {
                for (size_t j = 0; j < user_cryptomattes.count; j++) {
                    const char* user_aov_name = user_cryptomattes.aovs[j].c_str();
                    if (strcmp(aov_name, user_aov_name) == 0) {
                        // will be destroyed when cryptomatteData is
                        cryptoAOVs = AiArrayAllocate(option_aov_depth, 1, AI_TYPE_STRING);
                        driver = AiNodeLookUpByName(driver_name);
                        tmp_uc_drivers_vv[j].push_back(driver);
                        user_cryptomattes.aov_arrays[j] = cryptoAOVs;
                        break;
                    }
                }
            }

            if (cryptoAOVs != NULL) {
                for (uint32_t j = 0; j < option_aov_depth; j++)
                    AiArraySetStr(cryptoAOVs, j, "");
                create_AOV_array(aov_name, filter_name, camera_name, driver, cryptoAOVs,
                                 &new_outputs);
            }
        }

        if (new_outputs.size() > 0) {
            if (option_sidecar_manifests) {
                AtString manifest_driver_name("cryptomatte_manifest_driver");
                AtNode* manifest_driver = AiNodeLookUpByName(manifest_driver_name);
                if (manifest_driver == NULL) {
                    manifest_driver = AiNode("cryptomatte_manifest_driver");
                    AiNodeSetStr(manifest_driver, "name", manifest_driver_name);
                }
                AiNodeSetLocalData(manifest_driver, this);
                new_outputs.push_back(manifest_driver_name.c_str());
            }
            const uint32_t total_outputs = prev_output_num + (uint32_t)new_outputs.size();
            // Does not need destruction
            AtArray* final_outputs = AiArrayAllocate(total_outputs, 1, AI_TYPE_STRING);
            for (uint32_t i = 0; i < prev_output_num; i++) {
                // Iterate through old outputs and add them
                AiArraySetStr(final_outputs, i, AiArrayGetStr(outputs, i));
            }
            for (int i = 0; i < new_outputs.size(); i++) {
                // Iterate through new outputs and add them
                AiArraySetStr(final_outputs, i + prev_output_num, new_outputs[i].c_str());
            }
            AiNodeSetArray(renderOptions, "outputs", final_outputs);
        }

        build_standard_metadata(driver_cryptoAsset_v, driver_cryptoObject_v,
                                driver_cryptoMaterial_v);
        build_user_metadata(tmp_uc_drivers_vv);
    }

    void setup_deferred_manifest(AtNode* driver, AtString token, std::string& path_out,
                                 std::string& metadata_path_out) {
        path_out = "";
        metadata_path_out = "";
        if (check_driver(driver) && option_sidecar_manifests) {
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
        const bool do_md_asset = manif_asset_paths.size() > 0;
        const bool do_md_object = manif_object_paths.size() > 0;
        const bool do_md_material = manif_material_paths.size() > 0;

        if (!do_md_asset && !do_md_object && !do_md_material)
            return;

        ManifestMap map_md_asset, map_md_object, map_md_material;
        compile_standard_manifests(do_md_asset, do_md_object, do_md_material, map_md_asset,
                                   map_md_object, map_md_material);

        if (do_md_asset)
            write_manifest_sidecar_file(map_md_asset, manif_asset_paths);
        if (do_md_object)
            write_manifest_sidecar_file(map_md_object, manif_object_paths);
        if (do_md_material)
            write_manifest_sidecar_file(map_md_material, manif_material_paths);

        // reset sidecar writers
        manif_asset_paths = StringVector();
        manif_object_paths = StringVector();
        manif_material_paths = StringVector();
    }

    void compile_standard_manifests(bool do_md_asset, bool do_md_object, bool do_md_material,
                                    ManifestMap& map_md_asset, ManifestMap& map_md_object,
                                    ManifestMap& map_md_material) {
        AtNodeIterator* shape_iterator = AiUniverseGetNodeIterator(AI_NODE_SHAPE);
        while (!AiNodeIteratorFinished(shape_iterator)) {
            AtNode* node = AiNodeIteratorGetNext(shape_iterator);
            if (!node)
                continue;

            // skip any list aggregate nodes
            if (AiNodeIs(node, aStr_list_aggregate))
                continue;

            char nsp_name[MAX_STRING_LENGTH] = "";
            char obj_name[MAX_STRING_LENGTH] = "";

            get_object_names(NULL, node, option_strip_obj_ns, nsp_name, obj_name);

            if (do_md_asset || do_md_object) {
                add_obj_to_manifest(node, nsp_name, CRYPTO_ASSET_UDATA, CRYPTO_ASSET_OFFSET_UDATA,
                                    map_md_asset);
                add_obj_to_manifest(node, obj_name, CRYPTO_OBJECT_UDATA, CRYPTO_OBJECT_OFFSET_UDATA,
                                    map_md_object);
            }
            if (do_md_material) {
                // Process all shaders from the objects into the manifest.
                // This includes cluster materials.
                AtArray* shaders = AiNodeGetArray(node, "shader");
                if (!shaders)
                    continue;
                for (uint32_t i = 0; i < AiArrayGetNumElements(shaders); i++) {
                    char mat_name[MAX_STRING_LENGTH] = "";
                    AtNode* shader = static_cast<AtNode*>(AiArrayGetPtr(shaders, i));
                    if (!shader)
                        continue;
                    get_material_name(NULL, node, shader, option_strip_mat_ns, mat_name);
                    add_obj_to_manifest(node, mat_name, CRYPTO_MATERIAL_UDATA,
                                        CRYPTO_MATERIAL_OFFSET_UDATA, map_md_material);
                }
            }
        }
        AiNodeIteratorDestroy(shape_iterator);
    }

    void write_user_sidecar_manifests() {
        std::vector<bool> do_metadata;
        do_metadata.resize(user_cryptomattes.count);
        std::vector<ManifestMap> manf_maps;
        manf_maps.resize(user_cryptomattes.count);

        for (size_t i = 0; i < user_cryptomattes.count; i++) {
            do_metadata[i] = true;
            for (size_t j = 0; j < manifs_user_paths[i].size(); j++)
                do_metadata[i] = do_metadata[i] && manifs_user_paths[i][j].length() > 0;
        }
        compile_user_manifests(do_metadata, manf_maps);

        for (size_t i = 0; i < manifs_user_paths.size(); i++)
            if (do_metadata[i])
                write_manifest_sidecar_file(manf_maps[i], manifs_user_paths[i]);

        manifs_user_paths = std::vector<StringVector>();
    }

    void compile_user_manifests(std::vector<bool>& do_metadata,
                                std::vector<ManifestMap>& manf_maps) {
        if (user_cryptomattes.count == 0)
            return;
        AtNodeIterator* shape_iterator = AiUniverseGetNodeIterator(AI_NODE_SHAPE);
        while (!AiNodeIteratorFinished(shape_iterator)) {
            AtNode* node = AiNodeIteratorGetNext(shape_iterator);
            for (uint32_t i = 0; i < user_cryptomattes.count; i++) {
                if (do_metadata[i])
                    add_override_udata_to_manifest(node, user_cryptomattes.sources[i],
                                                   manf_maps[i]);
            }
        }
        AiNodeIteratorDestroy(shape_iterator);
    }

    void build_standard_metadata(const std::vector<AtNode*>& driver_asset_v,
                                 const std::vector<AtNode*>& driver_object_v,
                                 const std::vector<AtNode*>& driver_material_v) {
        clock_t metadata_start_time = clock();

        bool do_md_asset = false, do_md_object = false, do_md_material = false;
        for (auto& driver_asset : driver_asset_v) {
            if (metadata_needed(driver_asset, aov_cryptoasset)) {
                do_md_asset = true;
                metadata_set_unneeded(driver_asset, aov_cryptoasset);
                break;
            }
        }
        for (auto& driver_object : driver_object_v) {
            if (metadata_needed(driver_object, aov_cryptoobject)) {
                do_md_object = true;
                metadata_set_unneeded(driver_object, aov_cryptoobject);
                break;
            }
        }
        for (auto& driver_material : driver_material_v) {
            if (metadata_needed(driver_material, aov_cryptomaterial)) {
                do_md_material = true;
                metadata_set_unneeded(driver_material, aov_cryptomaterial);
                break;
            }
        }

        if (!do_md_asset && !do_md_object && !do_md_material)
            return;

        ManifestMap map_md_asset, map_md_object, map_md_material;

        if (!option_sidecar_manifests)
            compile_standard_manifests(do_md_asset, do_md_object, do_md_material, map_md_asset,
                                       map_md_object, map_md_material);

        std::string manif_asset_m, manif_object_m, manif_material_m;
        manif_asset_paths.resize(driver_asset_v.size());
        for (size_t i = 0; i < driver_asset_v.size(); i++) {
            setup_deferred_manifest(driver_asset_v[i], aov_cryptoasset, manif_asset_paths[i],
                                    manif_asset_m);
            write_metadata_to_driver(driver_asset_v[i], aov_cryptoasset, map_md_asset,
                                     manif_asset_m);
        }
        manif_object_paths.resize(driver_object_v.size());
        for (size_t i = 0; i < driver_object_v.size(); i++) {
            setup_deferred_manifest(driver_object_v[i], aov_cryptoobject, manif_object_paths[i],
                                    manif_object_m);
            write_metadata_to_driver(driver_object_v[i], aov_cryptoobject, map_md_object,
                                     manif_object_m);
        }
        manif_material_paths.resize(driver_material_v.size());
        for (size_t i = 0; i < driver_material_v.size(); i++) {
            setup_deferred_manifest(driver_material_v[i], aov_cryptomaterial,
                                    manif_material_paths[i], manif_material_m);
            write_metadata_to_driver(driver_material_v[i], aov_cryptomaterial, map_md_material,
                                     manif_material_m);
        }

        if (!option_sidecar_manifests)
            AiMsgInfo("Cryptomatte manifest created - %f seconds",
                      (float(clock() - metadata_start_time) / CLOCKS_PER_SEC));
        else
            AiMsgInfo("Cryptomatte manifest creation deferred - sidecar file "
                      "written at end of render.");
    }

    void build_user_metadata(const std::vector<std::vector<AtNode*>>& drivers_vv) {
        std::vector<StringVector> manifs_user_m;
        manifs_user_paths = std::vector<StringVector>();
        manifs_user_m.resize(drivers_vv.size());
        manifs_user_paths.resize(drivers_vv.size());

        const bool sidecar = option_sidecar_manifests;
        if (user_cryptomattes.count == 0 || drivers_vv.size() == 0)
            return;

        const clock_t metadata_start_time = clock();
        std::vector<bool> do_metadata;
        do_metadata.resize(user_cryptomattes.count);
        std::vector<ManifestMap> manf_maps;
        manf_maps.resize(user_cryptomattes.count);

        bool do_anything = false;
        for (uint32_t i = 0; i < drivers_vv.size(); i++) {
            do_metadata[i] = false;
            for (size_t j = 0; j < drivers_vv[i].size(); j++) {
                AtNode* driver = drivers_vv[i][j];
                AtString user_aov = user_cryptomattes.aovs[i];
                do_metadata[i] = do_metadata[i] || metadata_needed(driver, user_aov);
                do_anything = do_anything || do_metadata[i];

                std::string manif_user_m;
                if (sidecar) {
                    std::string manif_asset_paths;
                    setup_deferred_manifest(driver, user_aov, manif_asset_paths, manif_user_m);
                    manifs_user_paths[i].push_back(driver == NULL ? "" : manif_asset_paths);
                }
                manifs_user_m[i].push_back(driver == NULL ? "" : manif_user_m);
            }
        }

        if (!do_anything)
            return;

        if (!sidecar)
            compile_user_manifests(do_metadata, manf_maps);

        for (uint32_t i = 0; i < drivers_vv.size(); i++) {
            if (!do_metadata[i])
                continue;
            AtString aov_name = user_cryptomattes.aovs[i];
            for (size_t j = 0; j < drivers_vv[i].size(); j++) {
                AtNode* driver = drivers_vv[i][j];
                if (driver) {
                    metadata_set_unneeded(driver, aov_name);
                    write_metadata_to_driver(driver, aov_name, manf_maps[i], manifs_user_m[i][j]);
                }
            }
        }
        AiMsgInfo("User Cryptomatte manifests created - %f seconds",
                  float(clock() - metadata_start_time) / CLOCKS_PER_SEC);
    }

    void create_AOV_array(const char* aov_name, const char* filter_name, const char* camera_name,
                          AtNode* driver, AtArray* cryptoAOVs, StringVector* new_ouputs) {
        // helper for setup_cryptomatte_nodes. Populates cryptoAOVs and returns
        // the number of new outputs created.
        if (!check_driver(driver)) {
            AiMsgWarning("Cryptomatte Error: Can only write Cryptomatte to EXR files.");
            return;
        }

        ///////////////////////////////////////////////
        //      Compile info about original filter

        float aFilter_width = 2.0;
        char aFilter_filter[128];
        AtNode* orig_filter = AiNodeLookUpByName(filter_name);
        const AtNodeEntry* orig_filter_nodeEntry = AiNodeGetNodeEntry(orig_filter);
        const char* orig_filter_type_name = AiNodeEntryGetName(orig_filter_nodeEntry);
        if (AiNodeEntryLookUpParameter(orig_filter_nodeEntry, "width") != NULL) {
            aFilter_width = AiNodeGetFlt(orig_filter, "width");
        }

        memset(aFilter_filter, 0, sizeof(aFilter_filter));
        size_t filter_name_len = strlen(orig_filter_type_name);
        strncpy(aFilter_filter, orig_filter_type_name, filter_name_len);
        char* filter_strip_point = strstr(aFilter_filter, "_filter");
        if (filter_strip_point != NULL) {
            filter_strip_point[0] = '\0';
        }

        ///////////////////////////////////////////////
        //      Set CryptoAOV driver to full precision and outlaw RLE

        AiNodeSetBool(driver, "half_precision", false);

        const AtEnum compressions =
            AiParamGetEnum(AiNodeEntryLookUpParameter(AiNodeGetNodeEntry(driver), "compression"));
        const int compression = AiNodeGetInt(driver, "compression");
        const bool cmp_rle = compression == AiEnumGetValue(compressions, "rle"),
                   cmp_dwa = compression == AiEnumGetValue(compressions, "dwaa") ||
                             compression == AiEnumGetValue(compressions, "dwab");
        if (cmp_rle)
            AiMsgWarning("Cryptomatte cannot be set to RLE compression- it "
                         "does not work on full float. Switching to Zip.");
        if (cmp_dwa)
            AiMsgWarning("Cryptomatte cannot be set to dwa compression- the "
                         "compression breaks Cryptomattes. Switching to Zip.");
        if (cmp_rle || cmp_dwa)
            AiNodeSetStr(driver, "compression", "zip");

        AiAOVRegister(aov_name, AI_TYPE_RGB, AI_AOV_BLEND_OPACITY);

        AtArray* outputs = AiNodeGetArray(AiUniverseGetOptions(), "outputs");

        std::unordered_set<std::string> outputSet;
        for (uint32_t i = 0; i < AiArrayGetNumElements(outputs); i++)
            outputSet.insert(std::string(AiArrayGetStr(outputs, i)));

        std::unordered_set<std::string> splitAOVs;
        ///////////////////////////////////////////////
        //      Create filters and outputs as needed
        for (int i = 0; i < option_aov_depth; i++) {
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

            strcat(filter_rank_name, "_filter");
            strcat(filter_rank_name, rank_number_string);
            strcat(aov_rank_name, rank_number_string);

            const bool nofilter = AiNodeLookUpByName(filter_rank_name) == NULL;
            if (nofilter) {
                AtNode* filter = AiNode("cryptomatte_filter");
                AiNodeSetStr(filter, "name", filter_rank_name);
                AiNodeSetInt(filter, "rank", i * 2);
                AiNodeSetStr(filter, "filter", aFilter_filter);
                AiNodeSetFlt(filter, "width", aFilter_width);
                AiNodeSetStr(filter, "mode", "double_rgba");
            }

            std::string new_output_str;
            if (camera_name != NULL)
                new_output_str += std::string(camera_name) + " ";
            new_output_str += aov_rank_name;
            new_output_str += " ";
            new_output_str += "FLOAT";
            new_output_str += " ";
            new_output_str += filter_rank_name;
            new_output_str += " ";
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
        if (aov_array_cryptoasset)
            AiArrayDestroy(aov_array_cryptoasset);
        if (aov_array_cryptoobject)
            AiArrayDestroy(aov_array_cryptoobject);
        if (aov_array_cryptomaterial)
            AiArrayDestroy(aov_array_cryptomaterial);
        aov_array_cryptoasset = NULL;
        aov_array_cryptoobject = NULL;
        aov_array_cryptomaterial = NULL;
        user_cryptomattes = UserCryptomattes();
    }

public:
    ~CryptomatteData() { destroy_arrays(); }
};
