// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "common/XSIMDTypes.hpp"

#include <xsimd/xsimd.hpp>

#include <cmath>
#include <complex>
#include <type_traits>

// Actually enforce IsRootSupported
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
    struct IsComplex : std::false_type {};

    template <typename T>
    struct IsComplex<std::complex<T>> : std::true_type {};

    template <typename T>
    struct IsSIMDRootSupported : std::integral_constant<bool,
        XSIMDTraits<T>::IsSupported &&
        std::is_floating_point<T>::value &&
        !IsComplex<T>::value> {};

    template <typename T>
    using EnableForSIMDRoot = typename std::enable_if<IsSIMDRootSupported<T>::value>::type;

    template <typename T>
    using EnableForDefaultRoot = typename std::enable_if<!IsSIMDRootSupported<T>::value>::type;

#define ROOT_FCN(func) \
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
    static EnableForSIMDRoot<T> func(const T* in, T* out, size_t len) \
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
    static inline EnableForDefaultRoot<T> func(const T* in, T* out, size_t len) \
    { \
        func ## Unoptimized(in, out, len); \
    }

    ROOT_FCN(sqrt)
    ROOT_FCN(cbrt)
}

// Don't expose the SFINAE
#define DEFINE_FUNC(func) \
    template <typename T> \
    void func(const T* in, T* out, size_t len) \
    { \
        detail::func(in, out, len); \
    }

DEFINE_FUNC(sqrt)
DEFINE_FUNC(cbrt)

#define SPECIALIZE_FUNCS(T) \
    template void sqrt<T>(const T*, T*, size_t); \
    template void cbrt<T>(const T*, T*, size_t); \

SPECIALIZE_FUNCS(float)
SPECIALIZE_FUNCS(double)

}}
