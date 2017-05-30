#include <ai.h>
#include <cstring>

typedef void (*NodeRegisterFunc)(AtNodeLib *node);

void registerCryptomatteAov(AtNodeLib *node);
void registerCryptomatteFilter(AtNodeLib *node);

static NodeRegisterFunc registry[] =
{
    &registerCryptomatteAov,
    &registerCryptomatteFilter,
};

static const int num_nodes = sizeof(registry) / sizeof(NodeRegisterFunc);

node_loader
{
    if (i >= num_nodes) return false;

    strcpy(node->version, AI_VERSION);
    (*registry[i])(node);

    return true;
}
