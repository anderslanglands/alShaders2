#include "MurmurHash3.h"
#include <ai.h>
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

#include "cryptomatte.h"

#define NOMINMAX // lets you keep using std::min

///////////////////////////////////////////////
//
//      Constants
//
///////////////////////////////////////////////

// User data names
const AtString CRYPTO_ASSET_UDATA("crypto_asset");
const AtString CRYPTO_OBJECT_UDATA("crypto_object");
const AtString CRYPTO_MATERIAL_UDATA("crypto_material");

const AtString CRYPTO_ASSET_OFFSET_UDATA("crypto_asset_offset");
const AtString CRYPTO_OBJECT_OFFSET_UDATA("crypto_object_offset");
const AtString CRYPTO_MATERIAL_OFFSET_UDATA("crypto_material_offset");

// Some static AtStrings to cache
const AtString aStr_shader("shader");
const AtString aStr_listAggregate("list_aggregate");

uint8_t g_pointcloud_instance_verbosity = 0; // to do: remove this.

///////////////////////////////////////////////
//
//      Cryptomatte Cache
//
///////////////////////////////////////////////
CryptomatteCache CRYPTOMATTE_CACHE[AI_MAX_THREADS];

///////////////////////////////////////////////
//
//      CRYPTOMATTE UTILITIES
//
///////////////////////////////////////////////

///////////////////////////////////////////////
//
//      Shader Data Struct
//
///////////////////////////////////////////////

///////////////////////////////////////////////
//
//      Include test code
//
///////////////////////////////////////////////

#include "cryptomatte_tests.h"

///////////////////////////////////////////////
//
//      Called in shaders/drivers
//
///////////////////////////////////////////////

// CryptomatteData* CryptomatteData_new(AtNode *node) {
//     run_all_unit_tests(node);
//     return new CryptomatteData;
// }
// void CryptomatteData_setup_all(CryptomatteData *data, const AtString
// aov_cryptoasset,
//                                const AtString aov_cryptoobject, const
//                                AtString aov_cryptomaterial,
//                                AtArray *uc_aov_array, AtArray *uc_src_array)
//                                {
//     data->setup_all(aov_cryptoasset, aov_cryptoobject, aov_cryptomaterial,
//     uc_aov_array, uc_src_array);
// }
// void CryptomatteData_set_option_channels(CryptomatteData *data, int depth,
// bool previewchannels) {
//     data->set_option_channels(depth, previewchannels);
// }
// void CryptomatteData_set_option_namespace_stripping(CryptomatteData *data,
// bool strip_obj, bool strip_mat) {
//     data->set_option_namespace_stripping(strip_obj, strip_mat);
// }
// void CryptomatteData_set_option_ice_pcloud_verbosity(CryptomatteData *data,
// int verbosity) {
//     data->set_option_ice_pcloud_verbosity(verbosity);
// }
// void CryptomatteData_set_option_sidecar_manifests(CryptomatteData *data, bool
// sidecar) {
//     data->set_option_sidecar_manifests(sidecar);
// }
// void CryptomatteData_do_cryptomattes(CryptomatteData *data, AtShaderGlobals
// *sg ) {
//     data->do_cryptomattes(sg);
// }
// void CryptomatteData_write_sidecar_manifests(CryptomatteData *data) {
//     data->write_sidecar_manifests();
// }
// void CryptomatteData_del(CryptomatteData *data) {
//     delete data;
// }
