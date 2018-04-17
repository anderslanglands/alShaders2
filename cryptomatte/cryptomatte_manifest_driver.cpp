#include "cryptomatte.h"
#include <ai.h>

AI_DRIVER_NODE_EXPORT_METHODS(CryptomatteManifestDriverMtd);

node_parameters { AiParameterStr("filename", "dummy.json"); }

node_initialize {
    static const char* required_aovs[] = {"FLOAT A", nullptr};
    AiRawDriverInitialize(node, required_aovs, false);
}

driver_needs_bucket { return false; }

driver_process_bucket {}

node_update {}

driver_supports_pixel_type { return true; }

driver_open {}

driver_extension {
    static const char* extensions[] = {nullptr};
    return extensions;
}

driver_prepare_bucket {}

driver_write_bucket {}

driver_close {
    CryptomatteData* data = (CryptomatteData*)AiNodeGetLocalData(node);
    if (data)
        data->write_sidecar_manifests();
}

node_finish {}

void registerCryptomatteManifestDriver(AtNodeLib* node) {
    node->methods = (AtNodeMethods*)CryptomatteManifestDriverMtd;
    node->output_type = AI_TYPE_NONE;
    node->name = "cryptomatte_manifest_driver";
    node->node_type = AI_NODE_DRIVER;
    strcpy(node->version, AI_VERSION);
}
