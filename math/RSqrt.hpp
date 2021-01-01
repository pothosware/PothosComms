// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

namespace detail
{
    // http://rrrola.wz.cz/inv_sqrt.html
    static float Q_rsqrt(float f)
    {
        static_assert(sizeof(uint32_t) == sizeof(float), "sizeof(uint32_t) != sizeof(float)");

        uint32_t u = 0;
        float f2 = 0.0f;
        std::memcpy(&u, &f, sizeof(f));

        u = 0x5F1FFFF9ul - (u >> 1);
        std::memcpy(&f2, &u, sizeof(f2));

        return 0.703952253f * f2 * (2.38924456f - f * f2 * f2);
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
