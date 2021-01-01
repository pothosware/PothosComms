// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "common/XSIMDTypes.hpp"

#include <xsimd/xsimd.hpp>

#include <cmath>
#include <type_traits>

// Actually enforce IsGammaSupported
#ifdef _MSC_VER
#pragma warning(error: 4667) // no function template defined that matches forced instantiation
#endif

#if !defined POTHOS_SIMD_NAMESPACE
#error Must define POTHOS_SIMD_NAMESPACE to build this file
#endif

namespace PothosCommsSIMD { namespace POTHOS_SIMD_NAMESPACE {

namespace detail
{
    template <typename T, typename Ret>
    using EnableForSIMDGamma = EnableIfXSIMDSupports<T, Ret>;

    template <typename T, typename Ret>
    using EnableForDefaultGamma = EnableIfXSIMDDoesNotSupport<T, Ret>;

    // XSIMD's float implementation for lgamma appears to be bugged and
    // outputs incorrect values, so fall back to our default implementation.
    template <typename T>
    struct IsSIMDLnGammaSupported: std::integral_constant<bool,
        XSIMDTraits<T>::IsSupported &&
        !std::is_same<T, float>::value> {};

    template <typename T, typename Ret>
    using EnableForSIMDLnGamma = typename std::enable_if<IsSIMDLnGammaSupported<T>::value, Ret>::type;

    template <typename T, typename Ret>
    using EnableForDefaultLnGamma = typename std::enable_if<!IsSIMDLnGammaSupported<T>::value, Ret>::type;

#define GAMMA_FCN(func,EnableForSIMD,EnableForDefault) \
    template <typename T> \
    static void func ## Unoptimized(const T* in, T* out, size_t len) \
    { \
        for (size_t elem = 0; elem < len; ++elem) \
        { \
            out[elem] = std::func(in[elem]); \
        } \
    } \
 \
    template <typename T> \
    static EnableForSIMD<T, void> func(const T* in, T* out, size_t len) \
    { \
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size; \
        const auto numSIMDFrames = len / simdSize; \
 \
        const T* inPtr = in; \
        T* outPtr = out; \
 \
        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex) \
        { \
            auto inReg = xsimd::load_unaligned(inPtr); \
            auto outReg = xsimd::func(inReg); \
            outReg.store_unaligned(outPtr); \
 \
            inPtr += simdSize; \
            outPtr += simdSize; \
        } \
 \
        func ## Unoptimized(inPtr, outPtr, (len - (inPtr - in))); \
    } \
 \
    template <typename T> \
    static inline EnableForDefault<T, void> func(const T* in, T* out, size_t len) \
    { \
        func ## Unoptimized(in, out, len); \
    }

    GAMMA_FCN(tgamma, EnableForSIMDGamma, EnableForDefaultGamma)
    GAMMA_FCN(lgamma, EnableForSIMDLnGamma, EnableForDefaultLnGamma)
}

// Don't expose the SFINAE
#define DEFINE_FUNC(func) \
    template <typename T> \
    void func(const T* in, T* out, size_t len) \
    { \
        detail::func(in, out, len); \
    } \
    template void func<float>(const float*, float*, size_t); \
    template void func<double>(const double*, double*, size_t);

DEFINE_FUNC(tgamma)
DEFINE_FUNC(lgamma)

}}
