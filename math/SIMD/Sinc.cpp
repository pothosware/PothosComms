// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <xsimd/xsimd.hpp>

#include <cmath>
#include <type_traits>

// Actually enforce EnableSincSIMD
#ifdef _MSC_VER
#pragma warning(error: 4667) // no function template defined that matches forced instantiation
#endif

// Only support scalar floating-point types.
template <typename T>
using EnableSincSIMD = typename std::enable_if<std::is_floating_point<T>::value>::type;

#if !defined POTHOS_SIMD_NAMESPACE
#error Must define POTHOS_SIMD_NAMESPACE to build this file
#endif

namespace PothosCommsSIMD { namespace POTHOS_SIMD_NAMESPACE {

    template <typename T>
    EnableSincSIMD<T> sinc(const T* in, T* out, size_t len)
    {
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size;
        const auto numSIMDFrames = len / simdSize;

        const T* inPtr = in;
        T* outPtr = out;

        static const auto ZeroReg = xsimd::batch<T, simdSize>(T(0.0));
        static const auto OneReg  = xsimd::batch<T, simdSize>(T(1.0));

        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex)
        {
            auto inReg = xsimd::load_unaligned(inPtr);
            auto outReg = xsimd::select((inReg == ZeroReg), OneReg, (xsimd::sin(inReg) / inReg));
            outReg.store_unaligned(outPtr);

            inPtr += simdSize;
            outPtr += simdSize;
        }

        for (size_t elem = (simdSize * numSIMDFrames); elem < len; ++elem)
        {
            #define ZERO(x) (std::abs(x) < 1e-6)
            out[elem] = ZERO(in[elem]) ? 1 : (std::sin(in[elem]) / in[elem]);
        }
    }

#define SINC(T) template void sinc(const T*, T*, size_t);

    SINC(float)
    SINC(double)

}}