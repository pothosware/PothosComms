// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <xsimd/xsimd.hpp>

#include <complex>
#include <type_traits>

// Actually enforce EnableIf(Not)Complex
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
    using EnableIfNotComplex = typename std::enable_if<!IsComplex<T>::value>::type;

    template <typename T>
    using EnableIfComplex = typename std::enable_if<IsComplex<T>::value>::type;

#define XSIMD_SCALAR_ARITHMETIC_FUNC(func, op) \
    template <typename T> \
    static EnableIfNotComplex<T> func(const T* in0, const T* in1, T* out, size_t len) \
    { \
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size; \
        const auto numSIMDFrames = len / simdSize; \
 \
        const T* inPtr0 = in0; \
        const T* inPtr1 = in1; \
        T* outPtr = out; \
 \
        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex) \
        { \
            auto inReg0 = xsimd::load_unaligned(inPtr0); \
            auto inReg1 = xsimd::load_unaligned(inPtr1); \
            auto outReg = inReg0 op inReg1; \
            outReg.store_unaligned(outPtr); \
 \
            inPtr0 += simdSize; \
            inPtr1 += simdSize; \
            outPtr += simdSize; \
        } \
 \
        for (size_t elem = (simdSize * numSIMDFrames); elem < len; ++elem) \
        { \
            out[elem] = in0[elem] op in1[elem]; \
        } \
    }

#define XSIMD_COMPLEX_ARITHMETIC_FUNC(func) \
    template <typename T> \
    static EnableIfComplex<T> func(const T* in0, const T* in1, T* out, size_t len) \
    { \
        using ScalarType = typename T::value_type; \
        func<ScalarType>((const ScalarType*)in0, (const ScalarType*)in1, (ScalarType*)out, (len * 2)); \
    }

    XSIMD_SCALAR_ARITHMETIC_FUNC(add, +)
    XSIMD_SCALAR_ARITHMETIC_FUNC(sub, -)
    XSIMD_SCALAR_ARITHMETIC_FUNC(mul, *)
    XSIMD_SCALAR_ARITHMETIC_FUNC(div, /)

    // Complex addition and subtraction operate independently on the real and imaginary components, so
    // we can just cast them to the scalar types and operate on those. Complex multiplication and addition
    // are not independent, and SIMD complex types batch all real components, then all imaginary components,
    // so we cannot optimize these with SIMD, as rearranging them, operating, and restoring is slower than
    // the normal STL operators.
    XSIMD_COMPLEX_ARITHMETIC_FUNC(add)
    XSIMD_COMPLEX_ARITHMETIC_FUNC(sub)
}

// Don't expose the SFINAE
#define DEFINE_FUNC(func) \
    template <typename T> \
    void func(const T* in0, const T* in1, T* out, size_t len) \
    { \
        detail::func(in0, in1, out, len); \
    }

DEFINE_FUNC(add)
DEFINE_FUNC(sub)
DEFINE_FUNC(mul)
DEFINE_FUNC(div)

#define SPECIALIZE_FUNCS(T) \
    template void add<T>(const T*, const T*, T*, size_t); \
    template void add<std::complex<T>>(const std::complex<T>*, const std::complex<T>*, std::complex<T>*, size_t); \
    template void sub<T>(const T*, const T*, T*, size_t); \
    template void sub<std::complex<T>>(const std::complex<T>*, const std::complex<T>*, std::complex<T>*, size_t); \
    template void mul<T>(const T*, const T*, T*, size_t); \
    template void div<T>(const T*, const T*, T*, size_t); \

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
