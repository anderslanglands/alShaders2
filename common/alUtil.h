#pragma once

#include <cstring>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <climits>
#include <cfloat>

#include <ai.h>

#define IMPORTANCE_EPS 1e-5f

inline AtRGB rgb(float f)
{
    AtRGB c;
    c.r = c.g = c.b = f;
    return c;
}

inline AtRGB rgb(float r, float g, float b)
{
    AtRGB c;
    c.r = r; c.g = g; c.b = b;
    return c;
}

inline AtRGB rgb(const AtVector& v)
{
    AtRGB c;
    c.r = v.x; c.g = v.y; c.b = v.z;
    return c;
}

inline AtVector aivec(float x, float y, float z)
{
    AtVector v; AiV3Create(v, x, y, z);
    return v;
}

inline AtVector aivec(float x)
{
    AtVector v; AiV3Create(v, x, x, x);
    return v;
}

inline AtRGB max(const AtRGB& c1, const AtRGB& c2)
{
    AtRGB c;
    c.r = std::max(c1.r, c2.r);
    c.g = std::max(c1.g, c2.g);
    c.b = std::max(c1.b, c2.b);
    return c;
}

inline AtRGB min(const AtRGB& c1, const AtRGB& c2)
{
    AtRGB c;
    c.r = std::min(c1.r, c2.r);
    c.g = std::min(c1.g, c2.g);
    c.b = std::min(c1.b, c2.b);
    return c;
}

inline AtVector max(const AtVector& c1, const AtVector& c2)
{
    AtVector c;
    c.x = std::max(c1.x, c2.x);
    c.y = std::max(c1.y, c2.y);
    c.z = std::max(c1.z, c2.z);
    return c;
}

inline AtVector min(const AtVector& c1, const AtVector& c2)
{
    AtVector c;
    c.x = std::min(c1.x, c2.x);
    c.y = std::min(c1.y, c2.y);
    c.z = std::min(c1.z, c2.z);
    return c;
}

inline int clamp(int a, int mn, int mx)
{
    return std::min(std::max(a, mn), mx);
}

inline float clamp(float a, float mn, float mx)
{
    return std::min(std::max(a, mn), mx);
}

inline AtRGB clamp(const AtRGB& a, const AtRGB& mn, const AtRGB& mx)
{
    return min(max(a, mn), mx);
}

inline AtVector clamp(const AtVector& a, const AtVector& mn, const AtVector& mx)
{
    return min(max(a, mn), mx);
}

inline AtVector fabs(const AtVector& v)
{
    AtVector r; AiV3Create(r, fabsf(v.x), fabsf(v.y), fabsf(v.z));
    return r;
}

inline AtRGB fabs(const AtRGB& c)
{
    return rgb(fabsf(c.r), fabsf(c.g), fabsf(c.b));
}

inline float maxh(const AtRGB& c)
{
   return std::max(std::max(c.r, c.g), c.b);
}

inline float minh(const AtRGB& c)
{
   return std::min(std::min(c.r, c.g ), c.b);
}


inline float lerp(const float a, const float b, const float t)
{
   return (1-t)*a + t*b;
}

inline AtRGB lerp(const AtRGB& a, const AtRGB& b, const float t)
{
   AtRGB r;
   r.r = lerp( a.r, b.r, t );
   r.g = lerp( a.g, b.g, t );
   r.b = lerp( a.b, b.b, t );
   return r;
}

inline AtVector lerp(const AtVector& a, const AtVector& b, const float t)
{
   AtVector r;
   r.x = lerp( a.x, b.x, t );
   r.y = lerp( a.y, b.y, t );
   r.z = lerp( a.z, b.z, t );
   return r;
}

inline AtRGBA lerp(const AtRGBA& a, const AtRGBA& b, const float t)
{
   AtRGBA r;
   r.r = lerp( a.r, b.r, t );
   r.g = lerp( a.g, b.g, t );
   r.b = lerp( a.b, b.b, t );
   r.a = lerp( a.a, b.a, t );
   return r;
}

inline std::ostream& operator<<(std::ostream& os, const AtRGB& c)
{
    os << "(" << c.r << ", " << c.g << ", " << c.b << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const AtVector2& c)
{
    os << "(" << c.x << ", " << c.y << ")";
    return os;
}


// concentricSampleDisk and cosineSampleHemisphere lifted from PBRT
/*
Copyright (c) 1998-2012, Matt Pharr and Greg Humphreys.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
inline void concentricSampleDisk(float u1, float u2, float& dx, float& dy)
{
    float r, theta;
    // Map uniform random numbers to $[-1,1]^2$
    float sx = 2 * u1 - 1;
    float sy = 2 * u2 - 1;

    // Map square to $(r,\theta)$

    // Handle degeneracy at the origin
    if (sx == 0.0 && sy == 0.0) {
        dx = 0.0;
        dy = 0.0;
        return;
    }
    if (sx >= -sy) {
        if (sx > sy) {
            // Handle first region of disk
            r = sx;
            if (sy > 0.0) theta = sy/r;
            else          theta = 8.0f + sy/r;
        }
        else {
            // Handle second region of disk
            r = sy;
            theta = 2.0f - sx/r;
        }
    }
    else {
        if (sx <= sy) {
            // Handle third region of disk
            r = -sx;
            theta = 4.0f - sy/r;
        }
        else {
            // Handle fourth region of disk
            r = -sy;
            theta = 6.0f + sx/r;
        }
    }
    theta *= float(AI_PI) / 4.f;
    dx = r * cosf(theta);
    dy = r * sinf(theta);
}

inline AtVector cosineSampleHemisphere(float u1, float u2)
{
   AtVector ret;
   concentricSampleDisk(u1, u2, ret.x, ret.z);
   ret.y = sqrtf(std::max(0.0f, 1.0f - ret.x*ret.x - ret.z*ret.z));
   return ret;
}

inline AtVector uniformSampleSphere(float u1, float u2) 
{
     float z = 1.f - 2.f * u1;
     float r = sqrtf(std::max(0.f, 1.f - z*z));
     float phi = 2.f * float(AI_PI) * u2;
     float x = r * cosf(phi);
     float y = r * sinf(phi);
     AtVector v;
    AiV3Create(v, x, y, z);
    return v;
}

inline AtVector uniformSampleHemisphere(float u1, float u2) {
    float y = u1;
    float r = sqrtf(MAX(0.f, 1.f - y*y));
    float phi = 2 * AI_PI * u2;
    float x = r * cosf(phi);
    float z = r * sinf(phi);
    return AiVector(x, y, z);
}

inline float uniformConePdf(float cosThetaMax) 
{
    return 1.f / (2.f * AI_PI * (1.f - cosThetaMax));
}


inline AtVector uniformSampleCone(float u1, float u2, float costhetamax) 
{
    float costheta = (1.f - u1) + u1 * costhetamax;
    float sintheta = sqrtf(1.f - costheta*costheta);
    float phi = u2 * 2.f * AI_PI;
    return aivec(cosf(phi) * sintheta, sinf(phi) * sintheta, costheta);
}


inline AtVector uniformSampleCone(float u1, float u2, float costhetamax,
        const AtVector &x, const AtVector &y, const AtVector &z) 
{
    float costheta = lerp(costhetamax, 1.f, u1);
    float sintheta = sqrtf(1.f - costheta*costheta);
    float phi = u2 * 2.f * AI_PI;
    return cosf(phi) * sintheta * x + sinf(phi) * sintheta * y +
        costheta * z;
}

inline float sphericalTheta(const AtVector &v)
{
    return acosf(clamp(v.z, -1.f, 1.f));
}


inline float sphericalPhi(const AtVector &v)
{
    float p = atan2f(v.y, v.x);
    return (p < 0.f) ? p + 2.f*float(AI_PI) : p;
}

inline float sphericalTheta(const AtVector& w, const AtVector& U)
{
    return acosf(clamp(AiV3Dot(U, w), -1.0f, 1.0f));
}

inline float sphericalPhi(const AtVector& w, const AtVector& V, const AtVector& W)
{
    return atan2f(AiV3Dot(w, V), AiV3Dot(w, W));
}

inline void sphericalDirection(float theta, float phi, const AtVector& U, const AtVector& V, const AtVector& W,
                                AtVector& w)
{
    w = U*cosf(theta)*sinf(phi) + V*cosf(theta)*cosf(phi) + W*sinf(theta);
}



inline float fresnel(float cosi, float etai)
{
    if (cosi >= 1.0f) return 0.0f;
    float sint = etai * sqrtf(1.0f-cosi*cosi);
    if ( sint >= 1.0f ) return 1.0f;

    float cost = sqrtf(1.0f-sint*sint);
    float pl =    (cosi - (etai * cost))
                    / (cosi + (etai * cost));
    float pp =    ((etai * cosi) - cost)
                    / ((etai * cosi) + cost);
    return (pl*pl+pp*pp)*0.5f;
}

inline float powerHeuristic(float fp, float gp)
{
    return SQR(fp) / (SQR(fp)+SQR(gp));
}

// Stolen wholesale from OSL:
// http://code.google.com/p/openshadinglanguage
/*
Copyright (c) 2009-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

inline float fresnel(float eta, const AtVector& N, const AtVector& I, AtVector& R, AtVector& T, bool& is_inside)
{
    float c = AiV3Dot(N,I);
    float neta;
    AtVector Nn;
    // compute reflection
    R = (2 * c) * N - I;
    // check which side of the surface we are on
    if (c > 0) {
        // we are on the outside of the surface, going in
        neta = 1 / eta;
        Nn   = N;
        is_inside = false;
    } else {
        // we are inside the surface,
        c  = -c;
        neta = eta;
        Nn   = -N;
        is_inside = true;
    }
    R = (2 * c) * Nn - I;
    float arg = 1 - (neta * neta * (1 - (c * c)));
    if (arg < 0) {
        T.x = T.y = T.z = 0.0f;
        return 1; // total internal reflection
    } else {
        float dnp = sqrtf(arg);
        float nK = (neta * c) - dnp;
        T = -(neta * I) + (nK * Nn);
        // compute Fresnel terms
        float cosTheta1 = c; // N.R
        float cosTheta2 = -AiV3Dot(Nn,T);
        float pPara = (cosTheta1 - eta * cosTheta2) / (cosTheta1 + eta * cosTheta2);
        float pPerp = (eta * cosTheta1 - cosTheta2) / (eta * cosTheta1 + cosTheta2);
        return 0.5f * (pPara * pPara + pPerp * pPerp);
    }
}

inline AtRGB sqrt(AtRGB c)
{
    c.r = sqrtf(c.r);
    c.g = sqrtf(c.g);
    c.b = sqrtf(c.b);
    return c;
}

inline AtRGB exp(AtRGB c)
{
    c.r = expf(c.r);
    c.g = expf(c.g);
    c.b = expf(c.b);
    return c;
}

inline AtRGB fast_exp(AtRGB c)
{
    c.r = fast_exp(c.r);
    c.g = fast_exp(c.g);
    c.b = fast_exp(c.b);
    return c;
}

inline AtRGB pow(AtRGB c, float e)
{
    c.r = c.r > 0 ? powf(c.r, e) : 0;
    c.g = c.g > 0 ? powf(c.g, e) : 0;
    c.b = c.b > 0 ? powf(c.b, e) : 0;
    return c;
}

inline AtRGB log(AtRGB c)
{
    c.r = logf(c.r);
    c.g = logf(c.g);
    c.b = logf(c.b);
    return c;
}

inline float luminance(const AtRGB& c)
{
    return c.r*0.212671f + c.g*0.715160f + c.b*0.072169f;
}

inline float luminance(float f)
{
    return f;
}

inline float contrast(float input, float contrast, float pivot)
{
    if (contrast == 1.0f) return input;

    return (input-pivot)*contrast + pivot;
}

inline AtRGB contrast(const AtRGB& input, float contrast, float pivot)
{
    if (contrast == 1.0f) return input;

    return AiColorCreate(
        (input.r-pivot)*contrast + pivot,
        (input.g-pivot)*contrast + pivot,
        (input.b-pivot)*contrast + pivot
    );
}

inline float bias(float f, float b)
{
    if (b > 0.0f) return powf(f, logf(b)/logf(0.5f));
    else return 0.0f;
}

inline float biasandgain(float f, float b, float g)
{
    if (f < 0.f) return f;

    if (b != 0.5f)
    {
        f = bias(f, b);
    }
    if (g != 0.5f)
    {
        if (f < 0.5f) f = 0.5f * bias(2.0f*f, 1.0f-g);
        else f = 1.0f - bias(2.0f - 2.0f*f, 1.0f-g)*0.5f;
    }
    return f;
}

// Adapted from OSL. See copyright notice above.
inline AtRGB rgb2hsv (AtRGB rgb)
{
    float r = rgb.r, g = rgb.g, b = rgb.b;
    float mincomp = std::min(r, std::min(g, b));
    float maxcomp = std::max(r, std::max(g, b));
    float delta = maxcomp - mincomp;  // chroma
    float h, s, v;
    v = maxcomp;
    if (maxcomp > 0)
        s = delta / maxcomp;
    else s = 0;
    if (s <= 0)
        h = 0;
    else
    {
        if      (r >= maxcomp) h = (g-b) / delta;
        else if (g >= maxcomp) h = 2 + (b-r) / delta;
        else                   h = 4 + (r-g) / delta;
        h /= 6;
        if (h < 0)
            h += 1;
    }
    return AiColorCreate(h, s, v);
}

// Adapted from OSL. See copyright notice above.
inline AtRGB hsv2rgb (const AtRGB& hsv)
{
    float h = hsv.r;
    float s = hsv.g;
    float v = hsv.b;

    if (s < 0.0001f)
    {
        return AiColorCreate(v, v, v);
    }
    else
    {
        h = 6.0f * (h - floorf(h));  // expand to [0..6)
        int hi = (int) h;
        float f = h - hi;
        float p = v * (1.0f-s);
        float q = v * (1.0f-s*f);
        float t = v * (1.0f-s*(1.0f-f));
        switch (hi)
        {
        case 0 : return AiColorCreate (v, t, p);
        case 1 : return AiColorCreate (q, v, p);
        case 2 : return AiColorCreate (p, v, t);
        case 3 : return AiColorCreate (p, q, v);
        case 4 : return AiColorCreate (t, p, v);
        default: return AiColorCreate (v, p, q);
        }
    }
}

// For the sake of simplicity we limit eta to 1.3 so cache A here
inline float A(float eta)
{
    float Fdr = -1.440f/(eta*eta) + 0.710f/eta + 0.668f + 0.0636f*eta;
    return (1 + Fdr)/(1 - Fdr);
}

inline AtRGB bssrdfbrdf( const AtRGB& _alpha_prime )
{
    AtRGB sq = sqrt( 3.0f * (AI_RGB_WHITE - _alpha_prime) );
    return _alpha_prime * 0.5f * (AI_RGB_WHITE + exp( -(A(1.3f)*rgb(4.0f/3.0f)*sq ) )) * exp( -sq );
}

void alphaInversion( const AtRGB& scatterColour, float scatterDist, AtRGB& sigma_s_prime_, AtRGB& sigma_a_ );
void alphaInversion(float sc, float& sigma_s_prime_, float& sigma_a_ );
float alpha1_3(float x);
inline AtRGB alpha1_3(const AtRGB& c)
{
    return rgb(alpha1_3(c.r), alpha1_3(c.g), alpha1_3(c.b));
}

inline std::ostream& operator<<(std::ostream& os, const AtVector& v)
{
   os << "(" << v.x << "," << v.y << "," << v.z << ")";
   return os;
}


/// Always-positive modulo function (assumes b > 0)
inline int modulo(int a, int b)
{
    int r = a % b;
    return (r < 0) ? r+b : r;
}

/// Always-positive modulo function, float version (assumes b > 0)
inline float modulo(float a, float b) {
    float r = fmodf(a, b);
    return (r < 0) ? r+b : r;
}

// TODO: make this better
#define M_RAN_INVM32 2.32830643653869628906e-010
inline double random(AtUInt32 ui) { return ui * M_RAN_INVM32; }

/// Stolen from https://github.com/imageworks/OpenShadingLanguage/blob/master/src/liboslexec/noiseimpl.h
/// hash an array of N 32 bit values into a pseudo-random value
/// based on my favorite hash: http://burtleburtle.net/bob/c/lookup3.c
/// templated so that the compiler can unroll the loops for us
template <int N>
inline unsigned int
inthash (const unsigned int k[N]) 
{
// define some handy macros
#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))
#define mix(a,b,c) \
{ \
a -= c; a ^= rot(c, 4); c += b; \
b -= a; b ^= rot(a, 6); a += c; \
c -= b; c ^= rot(b, 8); b += a; \
a -= c; a ^= rot(c,16); c += b; \
b -= a; b ^= rot(a,19); a += c; \
c -= b; c ^= rot(b, 4); b += a; \
}
#define final(a,b,c) \
{ \
c ^= b; c -= rot(b,14); \
a ^= c; a -= rot(c,11); \
b ^= a; b -= rot(a,25); \
c ^= b; c -= rot(b,16); \
a ^= c; a -= rot(c,4); \
b ^= a; b -= rot(a,14); \
c ^= b; c -= rot(b,24); \
}
    // now hash the data!
    unsigned int a, b, c, len = N;
    a = b = c = 0xdeadbeef + (len << 2) + 13;
    while (len > 3) {
        a += k[0];
        b += k[1];
        c += k[2];
        mix(a, b, c);
        len -= 3;
        k += 3;
    }
    switch (len) {
        case 3 : c += k[2];
        case 2 : b += k[1];
        case 1 : a += k[0];
        final(a, b, c);
        case 0:
            break;
    }
    return c;
    // macros not needed anymore
#undef rot
#undef mix
#undef final
}



/// return the greatest integer <= x
/// stolen from https://github.com/imageworks/OpenShadingLanguage/blob/master/src/liboslexec/noiseimpl.h
inline int quickFloor(float x) 
{
    return (int) x - ((x < 0) ? 1 : 0);
}

/// linear congruential generator.
/// stolen from https://github.com/imageworks/OpenShadingLanguage/blob/master/src/liboslexec/gabornoise.cpp
class LCG
{
public:
    LCG(const AtVector& P, AtUInt32 seed=0)
    {
        // Seed based on cell P is in
        AtUInt32 pi[4] = 
        {
            AtUInt32(floorf(P.x)),
            AtUInt32(floorf(P.y)),
            AtUInt32(floorf(P.z)),
            seed
        };
        _seed = inthash<4>(pi);
        if (!_seed) _seed = 1;
    }

    /// return float on [0,1)
    float operator()()
    {
        _seed *= 3039177861u;
        return float(_seed) / float(UINT_MAX);
    }

    /// poisson distribution
    float poisson(float mean)
    {
        float g = fast_exp(-mean);
        AtUInt32 em = 0;
        float t = (*this)();
        while (t > g)
        {
            ++em;
            t *= (*this)();
        }
        return float(em);
    }

private:
    AtUInt32 _seed;
};

// Polynomial solvers below adapted from Cortex (which appear to themselves have been hoisted from Imath)
// http://cortex-vfx.googlecode.com/svn-history/r2684/trunk/rsl/IECoreRI/Roots.h
//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2009, Image Engine Design Inc. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
//     * Neither the name of Image Engine Design nor the names of any
//       other contributors to this software may be used to endorse or
//       promote products derived from this software without specific prior
//       written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////
// Solves a * x + b == 0
inline float solveLinear(float a, float b, float& root)
{
    if (a != 0)
    {
        root = -b / a;
        return 1;
    }
    else if (b != 0)
    {
        return 0;
    }
    return -1;
}

inline float sign(float f)
{
    if (f < 0) return -1.0f;
    else return 1.0f;
}

inline float cubicRoot(float v)
{
    return sign(v)*powf(fabsf(v), 1/3);
}

inline float solveQuadratic(float a, float b, float c, float roots[3])
{
    float epsilon = 1e-16f;

    if (fabsf(a) < epsilon)
    {
        return solveLinear(b, c, roots[0]);
    }
    float D = b*b-4*a*c;

    if (fabsf(D) < epsilon)
    {
        roots[0] = -b/(2*a);
        return 1;
    }
    if (D > 0)
    {
        float s = sqrtf(D);
        roots[0] = (-b + s) / (2 * a);
        roots[1] = (-b - s) / (2 * a);
        return 2;
    }
    return 0;
}

// Computes real roots for a given cubic polynomial (x^3+Ax^2+Bx+C = 0).
// \todo: make sure it returns the same number of roots as in OpenEXR/ImathRoot.h
inline float solveNormalizedCubic(float A, float B, float C, float roots[3])
{
    float epsilon = 1e-16f;
    if (fabsf(C) < epsilon)
    {
        // 1 or 2 roots
        return solveQuadratic(1, A, B, roots);
    }

    float Q = (3*B - A*A)/9;
    float R = (9*A*B - 27*C - 2*A*A*A)/54;
    float D = Q*Q*Q + R*R;    // polynomial discriminant
    float rootCount = 1;

    if (D > 0) // complex or duplicate roots
    {
        float sqrtD = sqrtf(D);
        float S = cubicRoot( R + sqrtD );
        float T = cubicRoot( R - sqrtD );
        roots[0] = (-A/3 + (S + T));   // one real root
    }
    else  // 3 real roots
    {
        float th = acosf( R/sqrtf(-(Q*Q*Q)) );
        float sqrtQ = sqrtf(-Q);
        roots[0] = (2*sqrtQ*cosf(th/3) - A/3);
        roots[1] = (2*sqrtQ*cosf((th + 2*float(AI_PI))/3) - A/3);
        roots[2] = (2*sqrtQ*cosf((th + 4*float(AI_PI))/3) - A/3);
        rootCount = 3;
    }
    return rootCount;
}

inline float solveCubic(float a, float b, float c, float d, float roots[3])
{
    float epsilon = 1e-16f;
    if (fabsf(a) < epsilon)
    {
        return solveQuadratic (b, c, d, roots);
    }
    return solveNormalizedCubic (b / a, c / a, d / a, roots);
}

/// simultaneous sin and cos
inline void sincosf_(float x, float* sx, float* cx)
{
    *sx = sinf(x);
    *cx = sqrtf(1.0f - SQR(*sx));
}

inline float wrap(float s, float period)
{
    period = floorf(period);
    if (period < 1.0f) period = 1.0f;
    return s - period * floorf(s/period);
}

inline AtVector wrap(const AtVector& s, const AtVector& period)
{
    AtVector r;
    AiV3Create(r, wrap(s.x, period.x), wrap(s.y, period.y), wrap(s.z, period.z));
    return r;
}

inline AtVector floor(const AtVector& v)
{
    AtVector r;
    AiV3Create(r, floorf(v.x), floorf(v.y), floorf(v.z));
    return r;
}

inline bool AiIsFinite(const AtRGB& c)
{
    return AiIsFinite(c.r) && AiIsFinite(c.g) && AiIsFinite(c.b);
}

inline bool AiIsFinite(const AtVector& v)
{
    return AiIsFinite(v.x) && AiIsFinite(v.y) && AiIsFinite(v.z);
}

inline bool isValidColor(AtRGB c)
{
   return AiIsFinite(c) && c.r > -AI_EPSILON && c.g > -AI_EPSILON && c.b > -AI_EPSILON;
}

inline bool isPositiveReal(float f)
{
   return AiIsFinite(f) && f > -AI_EPSILON;
}

/// Generate a random integer in the range [0,n)
// TODO: this isn't great
inline int rand0n(int n)
{
    return rand() / (RAND_MAX / n + 1);
}


#define ALS_RAY_UNDEFINED 0
#define ALS_RAY_SSS 1
#define ALS_RAY_DUAL 2
#define ALS_RAY_HAIR 3

#define ALS_CONTEXT_NONE 0
#define ALS_CONTEXT_LAYER 1

#define VAR(x) #x << ": " << x
#define VARL(x) std::cerr << #x << ": " << x << "\n"
