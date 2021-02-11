// Copyright (c) 2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "Exp10.hpp"

#include <xsimd/xsimd.hpp>
#include <Pothos/Util/XSIMDTraits.hpp>

#include <cmath>
#include <complex>
#include <type_traits>

// Actually enforce EnableFor*
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
    struct IsSIMDExpSupported: std::integral_constant<bool,
        Pothos::Util::XSIMDTraits<T>::IsSupported &&
        std::is_floating_point<T>::value> {};

#define EXP_UNOPTIMIZED_FUNC(func) \
    template <typename T> \
    static void func ## Unoptimized(const T* in, T* out, size_t len) \
    { \
        for (size_t elem = 0; elem < len; ++elem) \
        { \
            out[elem] = std::func(in[elem]); \
        } \
    }

#define EXP_FUNC(func) \
    template <typename T> \
    static typename std::enable_if<IsSIMDExpSupported<T>::value, void>::type func(const T* in, T* out, size_t len) \
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
    static inline typename std::enable_if<!IsSIMDExpSupported<T>::value, void>::type func(const T* in, T* out, size_t len) \
    { \
        func ## Unoptimized(in, out, len); \
    }

    EXP_UNOPTIMIZED_FUNC(exp)
    EXP_UNOPTIMIZED_FUNC(exp2)
    EXP_UNOPTIMIZED_FUNC(expm1)

    template <typename T>
    static inline void exp10Unoptimized(const T* in, T* out, size_t len)
    {
        exp10Buffer(in, out, len);
    }

    template <typename T>
    static void expNUnoptimized(const T* in, T* out, T base, size_t len)
    {
        for (size_t elem = 0; elem < len; ++elem)
        {
            out[elem] = std::pow(T(base), in[elem]);
        }
    }

    EXP_FUNC(exp)
    EXP_FUNC(exp2)
    EXP_FUNC(exp10)
    EXP_FUNC(expm1)

    template <typename T>
    static typename std::enable_if<IsSIMDExpSupported<T>::value, void>::type expN(const T* in, T* out, T base, size_t len)
    {
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size;
        const auto numSIMDFrames = len / simdSize;

        const T* inPtr = in;
        T* outPtr = out;

        const auto expBaseReg = xsimd::exp(xsimd::batch<T, simdSize>(base));

        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex)
        {
            auto inReg = xsimd::load_unaligned(inPtr);
            auto outReg = xsimd::pow(expBaseReg, inReg);
            outReg.store_unaligned(outPtr);

            inPtr += simdSize;
            outPtr += simdSize;
        }

        // Perform the remaining operations manually.
        expNUnoptimized(inPtr, outPtr, base, (len - (inPtr - in)));
    }

    template <typename T>
    static typename std::enable_if<!IsSIMDExpSupported<T>::value, void>::type expN(const T* in, T* out, T base, size_t len)
    {
        expNUnoptimized(in, out, base, len);
    }
}

// Hide the SFINAE
#define DEFINE_FUNC(func) \
    template <typename T> \
    void func(const T* in, T* out, size_t num) \
    { \
        detail::func(in, out, num); \
    }

DEFINE_FUNC(exp)
DEFINE_FUNC(exp2)
DEFINE_FUNC(exp10)
DEFINE_FUNC(expm1)

// Different signature
template <typename T>
void expN(const T* in, T* out, T base, size_t num)
{
    detail::expN(in, out, base, num);
}

#define SPECIALIZE_FUNCS(T) \
    template void exp(const T*, T*, size_t); \
    template void exp2(const T*, T*, size_t); \
    template void exp10(const T*, T*, size_t); \
    template void expm1(const T*, T*, size_t); \
    template void expN(const T*, T*, T, size_t); \

    SPECIALIZE_FUNCS(std::int8_t)
    SPECIALIZE_FUNCS(std::int16_t)
    SPECIALIZE_FUNCS(std::int32_t)
    SPECIALIZE_FUNCS(std::int64_t)
    SPECIALIZE_FUNCS(std::uint8_t)
    SPECIALIZE_FUNCS(std::uint16_t)
    SPECIALIZE_FUNCS(std::uint32_t)
    SPECIALIZE_FUNCS(std::uint64_t)
    SPECIALIZE_FUNCS(float)
    SPECIALIZE_FUNCS(double)

}}
