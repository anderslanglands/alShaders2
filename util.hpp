#pragma once
#include <ai.h>

namespace a2 {

bool is_finite(float f) { return AiIsFinite(f); }

bool is_finite(AtRGB c) {
    return AiIsFinite(c.r) && AiIsFinite(c.g) && AiIsFinite(c.b);
}

bool is_positive(AtRGB c) { return c.r >= 0 && c.g >= 0 && c.b >= 0; }
}
