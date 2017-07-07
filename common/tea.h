#pragma once

#include <ai.h>

/**
* \file tea.h 
*/

/**
 * \ref Generate fast and reasonably good pseudorandom numbers using the
 * Tiny Encryption Algorithm (TEA) by David Wheeler and Roger Needham.
 *
 * For details, refer to "GPU Random Numbers via the Tiny Encryption Algorithm"
 * by Fahad Zafar, Marc Olano, and Aaron Curtis.
 *
 * \param v0
 *     First input value to be encrypted (could be the sample index)
 * \param v1
 *     Second input value to be encrypted (e.g. the requested random number dimension)
 * \param rounds
 *     How many rounds should be executed? Original default was 4, but this looks bad so
 *     we'll use 64 which has nicer properties
 * \return
 *     A uniformly distributed 64-bit integer
 */
inline AtUInt64 sampleTEA(AtUInt32 v0, AtUInt32 v1, int rounds = 64) {
    AtUInt32 sum = 0;

    for (int i=0; i<rounds; ++i)
    {
        sum += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xA341316C) ^ (v1 + sum) ^ ((v1 >> 5) + 0xC8013EA4);
        v1 += ((v0 << 4) + 0xAD90777D) ^ (v0 + sum) ^ ((v0 >> 5) + 0x7E95761E);
    }

    return ((AtUInt64) v1 << 32) + v0;
}

inline float sampleTEAFloat(AtUInt32 v0, AtUInt32 v1, int rounds = 64) {
    /* Trick from MTGP: generate an uniformly distributed
       single precision number in [1,2) and subtract 1. */
    union
    {
        AtUInt32 u;
        float f;
    } x;
    x.u = ((sampleTEA(v0, v1, rounds) & 0xFFFFFFFF) >> 9) | 0x3f800000UL;
    return x.f - 1.0f;
}