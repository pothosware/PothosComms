// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cmath>

namespace detail
{
    // https://en.wikipedia.org/wiki/Fast_inverse_square_root
    static float Q_rsqrt(float number)
    {	
        const float x2 = number * 0.5F;
        constexpr float threehalfs = 1.5F;

        union
        {
            float f;
            unsigned long i;
        } conv;

        conv.f  = number;
        conv.i = 0x5f3759df - ( conv.i >> 1 );
        conv.f *= ( threehalfs - ( x2 * conv.f * conv.f ) );

        return conv.f;
    }

    static void rsqrtBuffer(const float* in, float* out, size_t length)
    {
        for(size_t i = 0; i < length; ++i)
        {
            out[i] = Q_rsqrt(in[i]);
        }
    }

    static void rsqrtBuffer(const double* in, double* out, size_t length)
    {
        for(size_t i = 0; i < length; ++i)
        {
            out[i] = 1.0 / std::sqrt(in[i]);
        }
    }
}

template <typename T>
static void rsqrtBuffer(const T* in, T* out, size_t length)
{
    detail::rsqrtBuffer(in, out, length);
}
