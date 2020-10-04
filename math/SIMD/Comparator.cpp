// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <xsimd/xsimd.hpp>

#include <complex>
#include <type_traits>

// Actually enforce IsComparatorSupported
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
    struct IsComparatorSupported : std::integral_constant<bool,
        !std::is_same<T, std::int16_t>::value &&
        !std::is_same<T, std::uint16_t>::value> {};

    // Only build for scalar types
    template <typename T>
    using EnableIfSIMDSupported = typename std::enable_if<IsComparatorSupported<T>::value>::type;

#define COMPARATOR_FCN(func,op) \
    template <typename T> \
    static EnableIfSIMDSupported<T> func(const T* in0, const T* in1, char* out, const size_t len) \
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
        for (size_t elem = (simdSize * numSIMDFrames); elem < len; ++elem) \
        { \
            out[elem] = (in0[elem] op in1[elem]) ? 1 : 0; \
        } \
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
SPECIALIZE_FUNCS(std::int32_t)
SPECIALIZE_FUNCS(std::int64_t)
SPECIALIZE_FUNCS(std::uint8_t)
SPECIALIZE_FUNCS(std::uint32_t)
SPECIALIZE_FUNCS(std::uint64_t)
SPECIALIZE_FUNCS(float)
SPECIALIZE_FUNCS(double)

}}
