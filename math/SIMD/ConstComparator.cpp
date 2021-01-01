// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "common/XSIMDTypes.hpp"

#include <xsimd/xsimd.hpp>

#include <complex>
#include <type_traits>

// Actually enforce IsSIMDConstComparatorSupported
#ifdef _MSC_VER
#pragma warning(error: 4667) // no function template defined that matches forced instantiation
#endif

#if !defined POTHOS_SIMD_NAMESPACE
#error Must define POTHOS_SIMD_NAMESPACE to build this file
#endif

namespace PothosCommsSIMD { namespace POTHOS_SIMD_NAMESPACE {

namespace detail
{
    // No (u)int16 support due to XSIMD limitation
    template <typename T>
    struct IsSIMDConstComparatorSupported : std::integral_constant<bool,
        XSIMDTraits<T>::IsSupported &&
        !std::is_same<T, std::int16_t>::value &&
        !std::is_same<T, std::uint16_t>::value> {};

    template <typename T>
    using EnableForSIMDConstComparator = typename std::enable_if<IsSIMDConstComparatorSupported<T>::value>::type;

    template <typename T>
    using EnableForDefaultConstComparator = typename std::enable_if<!IsSIMDConstComparatorSupported<T>::value>::type;

#define COMPARATOR_FCN(func,op) \
    template <typename T> \
    static void func ## Unoptimized(const T* in, T val, char* out, size_t len) \
    { \
        for (size_t elem = 0; elem < len; ++elem) \
        { \
            out[elem] = (in[elem] op val) ? 1 : 0; \
        } \
    } \
 \
    template <typename T> \
    static EnableForSIMDConstComparator<T> func(const T* in, T val, char* out, size_t len) \
    { \
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size; \
        const auto numSIMDFrames = len / simdSize; \
 \
        const T* inPtr = in; \
        char* outPtr = out; \
 \
        static const auto ZeroReg = xsimd::batch<T, simdSize>(T(0)); \
        static const auto OneReg = xsimd::batch<T, simdSize>(T(1)); \
        const auto valReg = xsimd::batch<T, simdSize>(val); \
 \
        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex) \
        { \
            auto inReg = xsimd::load_unaligned(inPtr); \
            auto outReg = xsimd::select((inReg op valReg), OneReg, ZeroReg); \
            outReg.store_unaligned(outPtr); \
 \
            inPtr += simdSize; \
            outPtr += simdSize; \
        } \
 \
        func ## Unoptimized(inPtr, val, outPtr, (len - (inPtr - in))); \
    } \
 \
    template <typename T> \
    static EnableForDefaultConstComparator<T> func(const T* in, T val, char* out, size_t len) \
    { \
        func ## Unoptimized(in, val, out, len); \
    }

    COMPARATOR_FCN(constGreaterThan, >)
    COMPARATOR_FCN(constLessThan, <)
    COMPARATOR_FCN(constGreaterThanOrEqual, >=)
    COMPARATOR_FCN(constLessThanOrEqual, <=)
    COMPARATOR_FCN(constEqualTo, ==)
    COMPARATOR_FCN(constNotEqualTo, !=)
}

// Don't expose the SFINAE
#define DEFINE_FUNC(func) \
    template <typename T> \
    void func(const T* in, T val, char* out, size_t len) \
    { \
        detail::func(in, val, out, len); \
    }

DEFINE_FUNC(constGreaterThan)
DEFINE_FUNC(constLessThan)
DEFINE_FUNC(constGreaterThanOrEqual)
DEFINE_FUNC(constLessThanOrEqual)
DEFINE_FUNC(constEqualTo)
DEFINE_FUNC(constNotEqualTo)

#define SPECIALIZE_FUNCS(T) \
    template void constGreaterThan<T>(const T*, T, char*, size_t); \
    template void constLessThan<T>(const T*, T, char*, size_t); \
    template void constGreaterThanOrEqual<T>(const T*, T, char*, size_t); \
    template void constLessThanOrEqual<T>(const T*, T, char*, size_t); \
    template void constEqualTo<T>(const T*, T, char*, size_t); \
    template void constNotEqualTo<T>(const T*, T, char*, size_t);

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
