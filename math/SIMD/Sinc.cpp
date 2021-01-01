// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "common/XSIMDTypes.hpp"

#include <xsimd/xsimd.hpp>

#include <cmath>
#include <type_traits>

// Actually enforce EnableSincSIMD
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
    static void sincUnoptimized(const T* in, T* out, size_t len)
    {
        for (size_t elem = 0; elem < len; ++elem)
        {
            #define ZERO(x) (std::abs(x) < 1e-6)
            out[elem] = ZERO(in[elem]) ? 1 : (std::sin(in[elem]) / in[elem]);
        }
    }

    template <typename T>
    static EnableIfXSIMDSupports<T, void> sinc(const T* in, T* out, size_t len)
    {
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size;
        const auto numSIMDFrames = len / simdSize;

        const T* inPtr = in;
        T* outPtr = out;

        static const auto ZeroReg = xsimd::batch<T, simdSize>(T(0));
        static const auto OneReg  = xsimd::batch<T, simdSize>(T(1));

        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex)
        {
            auto inReg = xsimd::load_unaligned(inPtr);

            auto outReg = xsimd::select(
                               (inReg == ZeroReg),
                               OneReg,
                               (xsimd::sin(inReg) / inReg));
            outReg.store_unaligned(outPtr);

            inPtr += simdSize;
            outPtr += simdSize;
        }

        sincUnoptimized(inPtr, outPtr, (len - (inPtr - in)));
    }

    template <typename T>
    static inline EnableIfXSIMDDoesNotSupport<T, void> sinc(const T* in, T* out, size_t len)
    {
        sincUnoptimized(in, out, len);
    }
}

template <typename T>
void sinc(const T* in, T* out, size_t len)
{
    detail::sinc(in, out, len);
}

#define SINC(T) template void sinc(const T*, T*, size_t);

    SINC(float)
    SINC(double)

}}
