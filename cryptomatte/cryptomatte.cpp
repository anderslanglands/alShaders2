#include "cryptomatte.h"
#include "MurmurHash3.h"
#include <ai.h>

AtCritSec g_critsec;
bool g_critsec_active;
inline bool crypto_crit_sec_init();
inline void crypto_crit_sec_close();
inline void crypto_crit_sec_enter();
inline void crypto_crit_sec_leave();

// User data names
const AtString CRYPTO_ASSET_UDATA("crypto_asset");
const AtString CRYPTO_OBJECT_UDATA("crypto_object");
const AtString CRYPTO_MATERIAL_UDATA("crypto_material");

const AtString CRYPTO_ASSET_OFFSET_UDATA("crypto_asset_offset");
const AtString CRYPTO_OBJECT_OFFSET_UDATA("crypto_object_offset");
const AtString CRYPTO_MATERIAL_OFFSET_UDATA("crypto_material_offset");

uint8_t g_pointcloud_instance_verbosity = 0; // to do: remove this.

CryptomatteCache CRYPTOMATTE_CACHE[AI_MAX_THREADS];
