// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <xsimd/xsimd.hpp>
#include <Pothos/Util/XSIMDTraits.hpp>

#include <cmath>

// Actually enforce EnableIf*
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
    static void betaUnoptimized(const T* in0, const T* in1, T* out, size_t len)
    {
        for (size_t elem = 0; elem < len; ++elem)
        {
#if __cplusplus >= 201703L
            out[elem] = std::beta(in0[elem], in1[elem]);
#else
            out[elem] = std::exp(std::lgamma(in0[elem]) + std::lgamma(in1[elem]) - std::lgamma(in0[elem] + in1[elem]));
#endif
        }
    }

    template <typename T>
    static Pothos::Util::EnableIfXSIMDSupports<T, void> beta(const T* in0, const T* in1, T* out, size_t len)
    {
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size;
        const auto numSIMDFrames = len / simdSize;

        const T* in0Ptr = in0;
        const T* in1Ptr = in1;
        T* outPtr = out;

        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex)
        {
            auto in0Reg = xsimd::load_unaligned(in0Ptr);
            auto in1Reg = xsimd::load_unaligned(in1Ptr);
            auto outReg = xsimd::exp(xsimd::lgamma(in0Reg) + xsimd::lgamma(in1Reg) - xsimd::lgamma(in0Reg + in1Reg));
            outReg.store_unaligned(outPtr);

            in0Ptr += simdSize;
            in1Ptr += simdSize;
            outPtr += simdSize;
        }

        // Perform remaining operations manually.
        betaUnoptimized(in0Ptr, in1Ptr, outPtr, (len - (in0Ptr - in0)));
    }

    template <typename T>
    static Pothos::Util::EnableIfXSIMDDoesNotSupport<T, void> beta(const T* in0, const T* in1, T* out, size_t len)
    {
        betaUnoptimized(in0, in1, out, len);
    }
}

// Hide the SFINAE
template <typename T>
void beta(const T* in0, const T* in1, T* out, size_t len)
{
    detail::beta(in0, in1, out, len);
}

#define BETA(T) template void beta(const T*, const T*, T*, size_t);

    BETA(float)
    BETA(double)

}}
