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

inline std::ostream& operator<<(std::ostream& os, const AtMatrix& m) {
    for (int i = 0; i < 4; ++i) {
        os << "[";
        for (int j = 0; j < 4; ++j) {
            os << m.data[i][j] << ", ";
        }
        os << "]";
    }
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const AtShaderGlobals& sg) {
    os << "AtShaderGlobals {\n";
    os << "\t               x: " << sg.x << "\n";
    os << "\t               y: " << sg.y << "\n";
    os << "\t              px: " << sg.px << "\n";
    os << "\t              py: " << sg.py << "\n";
    os << "\t              si: " << sg.si << "\n";
    os << "\t    transp_index: " << sg.transp_index << "\n";
    os << "\t              Ro: " << sg.Ro << "\n";
    os << "\t              Rd: " << sg.Rd << "\n";
    os << "\t              Rl: " << sg.Rl << "\n";
    os << "\t             tid: " << sg.tid << "\n";
    os << "\t              Rt: " << sg.Rt << "\n";
    os << "\t         bounces: " << sg.bounces << "\n";
    os << "\t bounces_diffuse: " << sg.bounces_diffuse << "\n";
    os << "\tbounces_specular: " << sg.bounces_specular << "\n";
    os << "\t bounces_reflect: " << sg.bounces_reflect << "\n";
    os << "\tbounces_transmit: " << sg.bounces_transmit << "\n";
    os << "\t  bounces_volume: " << sg.bounces_volume << "\n";
    os << "\t           fhemi: " << sg.fhemi << "\n";
    os << "\t            time: " << sg.time << "\n";
    os << "\t              Op: " << sg.Op << "\n";
    os << "\t            proc: " << sg.proc << "\n";
    os << "\t          shader: " << sg.shader << "\n";
    os << "\t             psg: " << sg.psg << "\n";
    os << "\t              Po: " << sg.Po << "\n";
    os << "\t               P: " << sg.P << "\n";
    os << "\t            dPdx: " << sg.dPdx << "\n";
    os << "\t            dPdy: " << sg.dPdy << "\n";
    os << "\t               N: " << sg.N << "\n";
    os << "\t              Nf: " << sg.Nf << "\n";
    os << "\t              Ng: " << sg.Ng << "\n";
    os << "\t             Ngf: " << sg.Ngf << "\n";
    os << "\t              Ns: " << sg.Ns << "\n";
    os << "\t              bu: " << sg.bu << "\n";
    os << "\t              bv: " << sg.bv << "\n";
    os << "\t               u: " << sg.u << "\n";
    os << "\t               v: " << sg.v << "\n";
    os << "\t              fi: " << sg.fi << "\n";
    os << "\t               M: " << sg.M << "\n";
    os << "\t            Minv: " << sg.Minv << "\n";
    os << "\t         nlights: " << sg.nlights << "\n";
    os << "\t            dPdu: " << sg.dPdu << "\n";
    os << "\t            dPdv: " << sg.dPdv << "\n";
    os << "\t            dDdx: " << sg.dDdx << "\n";
    os << "\t            dDdy: " << sg.dDdy << "\n";
    os << "\t            dNdx: " << sg.dNdx << "\n";
    os << "\t            dNdy: " << sg.dNdy << "\n";
    os << "\t            dudx: " << sg.dudx << "\n";
    os << "\t            dudy: " << sg.dudy << "\n";
    os << "\t            dvdx: " << sg.dvdx << "\n";
    os << "\t            dvdy: " << sg.dvdy << "\n";
    os << "\t     skip_shadow: " << sg.skip_shadow << "\n";
    os << "\t              sc: " << sg.sc << "\n";

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
template <typename T> inline auto clamp(T x, T a, T b) -> T {
    if (x < a)
        return a;
    if (x > b)
        return b;
    return x;
}

template <typename T> inline auto lerp(T a, T b, float t) -> T {
    return (1 - t) * a + t * b;
}

template <typename T> inline auto sqr(T x) -> T { return x * x; }
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

inline float n_min(float r) { return (1 - r) / (1 + r); }

inline float n_max(float r) { return (1 + sqrt(r)) / (1 - sqrt(r)); }

inline float get_n(float r, float g) {
    return n_min(r) * g + (1 - g) * n_max(r);
}

inline float get_k2(float r, float n) {
    float nr = (n + 1) * (n + 1) * r - (n - 1) * (n - 1);
    return nr / (1 - r);
}
inline float get_r(float n, float k) {
    return ((n - 1) * (n - 1) + k * k) / ((n + 1) * (n + 1) + k * k);
}

inline float get_g(float n, float k) {
    float r = get_r(n, k);
    return (n_max(r) - n) / (n_max(r) - n_min(r));
}

inline float olefresnel(float r, float g, float c) {
    // clamp parameters
    float _r = clamp(r, 0.0f, 0.99f);
    // compute n and k
    float n = get_n(_r, g);
    float k2 = get_k2(_r, n);
    float rs_num = n * n + k2 - 2 * n * c + c * c;
    float rs_den = n * n + k2 + 2 * n * c + c * c;
    float rs = rs_num / rs_den;
    float rp_num = (n * n + k2) * c * c - 2 * n * c + 1;
    float rp_den = (n * n + k2) * c * c + 2 * n * c + 1;
    float rp = rp_num / rp_den;
    return 0.5 * (rs + rp);
}

inline AtRGB olefresnel(AtRGB r, AtRGB g, float mu) {
    return AtRGB(olefresnel(r.r, g.r, mu), olefresnel(r.g, g.g, mu),
                 olefresnel(r.b, g.b, mu));
}

inline AtRGB F_avg_ole(AtRGB r, AtRGB g) {
    return AtRGB(0.087237) + g * 0.0230685 - g * g * 0.0864902 +
           g * g * g * 0.07745948 + r * .782654 - r * r * 0.136432 +
           r * r * r * 0.278708 + g * r * 0.19744 + g * g * r * 0.0360605 -
           g * r * r * 0.2586;
}

inline float F_avg_dielectric(float eta) {
    if (eta == 1.0f)
        return 0.0f;
    if (eta <= 0.0f)
        return 1.0f;
    if (eta > 400.0f)
        return 1.0f;
    if (eta < 1) {
        return 0.997118 + 0.1014 * eta - 0.965241 * eta * eta -
               0.130607 * eta * eta * eta;
    } else {
        return (eta - 1) / (4.08567 + 1.00071 * eta);
    }
}

inline float dot(AtVector v1, AtVector v2) { return AiV3Dot(v1, v2); }
}
