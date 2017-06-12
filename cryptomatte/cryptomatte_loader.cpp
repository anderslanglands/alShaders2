#include <ai.h>
#include <cstring>

typedef void (*NodeRegisterFunc)(AtNodeLib *node);

void registerCryptomatte(AtNodeLib *node);
void registerCryptomatteFilter(AtNodeLib *node);
void registerCryptomatteManifestDriver(AtNodeLib *node);

static NodeRegisterFunc registry[] =
{
    &registerCryptomatte,
    &registerCryptomatteFilter,
    &registerCryptomatteManifestDriver,
};

static const int num_nodes = sizeof(registry) / sizeof(NodeRegisterFunc);

node_loader
{
    if (i >= num_nodes) return false;

    strcpy(node->version, AI_VERSION);
    (*registry[i])(node);

    return true;
}
