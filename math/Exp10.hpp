// Copyright (c) 2021 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cmath>

namespace detail
{
    //
    // There is no exp10 STL function for some reason, so
    // abstract it on this layer.
    //

    template <typename T>
    inline T exp10(T in)
    {
        return T(std::pow(T(10), in));
    }

    template <>
    inline float exp10(float in)
    {
        return ::exp10f(in);
    }

    template <>
    inline double exp10(double in)
    {
        return ::exp10(in);
    }
}

template <typename T>
static void exp10Buffer(const T* in, T* out, size_t length)
{
    for(size_t i = 0; i < length; ++i)
    {
        out[i] = detail::exp10(in[i]);
    }
}
