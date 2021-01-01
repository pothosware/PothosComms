// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "RSqrt.hpp"

#include "common/XSIMDTypes.hpp"

#include <xsimd/xsimd.hpp>

#include <cmath>
#include <type_traits>

// Actually enforce EnableRSqrtSIMD
#ifdef _MSC_VER
#pragma warning(error: 4667) // no function template defined that matches forced instantiation
#endif

#if !defined POTHOS_SIMD_NAMESPACE
#error Must define POTHOS_SIMD_NAMESPACE to build this file
#endif

namespace PothosCommsSIMD { namespace POTHOS_SIMD_NAMESPACE {

namespace detail
{
    template <typename T>
    static EnableIfXSIMDSupports<T, void> rsqrt(const T* in, T* out, size_t len)
    {
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size;
        const auto numSIMDFrames = len / simdSize;

        const T* inPtr = in;
        T* outPtr = out;

        static const auto OneReg  = xsimd::batch<T, simdSize>(T(1));

        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex)
        {
            auto inReg = xsimd::load_unaligned(inPtr);
            auto outReg = OneReg / xsimd::sqrt(inReg);
            outReg.store_unaligned(outPtr);

            inPtr += simdSize;
            outPtr += simdSize;
        }

        // Perform the remaining operations manually.
        rsqrtBuffer(inPtr, outPtr, (len - (outPtr - out)));
    }

    template <typename T>
    static EnableIfXSIMDDoesNotSupport<T, void> rsqrt(const T* in, T* out, size_t len)
    {
        rsqrtBuffer(in, out, len);
    }
}

template <typename T>
void rsqrt(const T* in, T* out, size_t len)
{
    detail::rsqrt(in, out, len);
}

#define RSQRT(T) template void rsqrt(const T*, T*, size_t);

    RSQRT(float)
    RSQRT(double)

}}
