// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "common/XSIMDTypes.hpp"

#include <xsimd/xsimd.hpp>

#include <complex>
#include <type_traits>

// Actually enforce EnableForSIMDComparator
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
    struct IsSIMDComparatorSupported: std::integral_constant<bool,
        XSIMDTraits<T>::IsSupported &&
        !std::is_same<T, std::int16_t>::value &&
        !std::is_same<T, std::uint16_t>::value> {};

    template <typename T>
    using EnableForSIMDComparator = typename std::enable_if<IsSIMDComparatorSupported<T>::value>::type;

    template <typename T>
    using EnableForDefaultComparator = typename std::enable_if<!IsSIMDComparatorSupported<T>::value>::type;

#define COMPARATOR_FCN(func,op) \
    template <typename T> \
    static void func ## Unoptimized(const T* in0, const T* in1, char* out, size_t len) \
    { \
        for (size_t elem = 0; elem < len; ++elem) \
        { \
            out[elem] = (in0[elem] op in1[elem]) ? 1 : 0; \
        } \
    } \
 \
    template <typename T> \
    static EnableForSIMDComparator<T> func(const T* in0, const T* in1, char* out, size_t len) \
    { \
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size; \
        const auto numSIMDFrames = len / simdSize; \
 \
        const T* in0Ptr = in0; \
        const T* in1Ptr = in1; \
        char* outPtr = out; \
 \
        static const auto ZeroReg = xsimd::batch<T, simdSize>(T(0)); \
        static const auto OneReg = xsimd::batch<T, simdSize>(T(1)); \
 \
        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex) \
        { \
            auto in0Reg = xsimd::load_unaligned(in0Ptr); \
            auto in1Reg = xsimd::load_unaligned(in1Ptr); \
            auto outReg = xsimd::select((in0Reg op in1Reg), OneReg, ZeroReg); \
            outReg.store_unaligned(outPtr); \
 \
            in0Ptr += simdSize; \
            in1Ptr += simdSize; \
            outPtr += simdSize; \
        } \
 \
        func ## Unoptimized(in0Ptr, in1Ptr, outPtr, (len - (in0Ptr - in0))); \
    } \
 \
    template <typename T> \
    static EnableForDefaultComparator<T> func(const T* in0, const T* in1, char* out, size_t len) \
    { \
        func ## Unoptimized(in0, in1, out, len); \
    }

    COMPARATOR_FCN(greaterThan, >)
    COMPARATOR_FCN(lessThan, <)
    COMPARATOR_FCN(greaterThanOrEqual, >=)
    COMPARATOR_FCN(lessThanOrEqual, <=)
    COMPARATOR_FCN(equalTo, ==)
    COMPARATOR_FCN(notEqualTo, !=)
}

// Don't expose the SFINAE
#define DEFINE_FUNC(func) \
    template <typename T> \
    void func(const T* in0, const T* in1, char* out, size_t len) \
    { \
        detail::func(in0, in1, out, len); \
    }

DEFINE_FUNC(greaterThan)
DEFINE_FUNC(lessThan)
DEFINE_FUNC(greaterThanOrEqual)
DEFINE_FUNC(lessThanOrEqual)
DEFINE_FUNC(equalTo)
DEFINE_FUNC(notEqualTo)

#define SPECIALIZE_FUNCS(T) \
    template void greaterThan<T>(const T*, const T*, char*, size_t); \
    template void lessThan<T>(const T*, const T*, char*, size_t); \
    template void greaterThanOrEqual<T>(const T*, const T*, char*, size_t); \
    template void lessThanOrEqual<T>(const T*, const T*, char*, size_t); \
    template void equalTo<T>(const T*, const T*, char*, size_t); \
    template void notEqualTo<T>(const T*, const T*, char*, size_t);

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
