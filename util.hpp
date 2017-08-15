#pragma once
#include <ai.h>
#include <iostream>

inline std::ostream& operator<<(std::ostream& os, AtRGB c) {
    os << "(" << c.r << ", " << c.g << ", " << c.b << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, AtVector v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

namespace a2 {

inline bool is_finite(float f) { return AiIsFinite(f); }

inline bool is_finite(AtRGB c) {
    return AiIsFinite(c.r) && AiIsFinite(c.g) && AiIsFinite(c.b);
}

inline bool is_positive(float f) { return f >= 0; }
inline bool is_positive(AtRGB c) { return c.r >= 0 && c.g >= 0 && c.b >= 0; }

inline AtRGB expf(AtRGB c) {
    return AtRGB(::expf(c.r), ::expf(c.g), ::expf(c.b));
}
inline float mix(float a, float b, float t) { return (1.0f - t) * a + t * b; }
inline float fresnel(float cosi, float etai) {
    if (cosi >= 1.0f)
        return 0.0f;
    auto sint = etai * sqrtf(1.0f - cosi * cosi);
    if (sint >= 1.0f)
        return 1.0f;

    auto cost = sqrtf(1.0f - sint * sint);
    auto pl = (cosi - (etai * cost)) / (cosi + (etai * cost));
    auto pp = ((etai * cosi) - cost) / ((etai * cosi) + cost);
    return (pl * pl + pp * pp) * 0.5f;
}

inline float dot(AtVector v1, AtVector v2) { return AiV3Dot(v1, v2); }
}
