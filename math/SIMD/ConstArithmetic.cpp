// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <xsimd/xsimd.hpp>

#include <complex>
#include <type_traits>

// Actually enforce EnableIfNotComplex
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

    // Make extra sure we're not trying to expose complex functions, since SIMD
    // representations of complex numbers do not match std::complex.
    template <typename T>
    using EnableIfNotComplex = typename std::enable_if<!IsComplex<T>::value>::type;

#define X_BY_K_FUNC(func,op) \
    template <typename T> \
    static EnableIfNotComplex<T> func(const T* in, const T& K, T* out, size_t len) \
    { \
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size; \
        const auto numSIMDFrames = len / simdSize; \
 \
        const T* inPtr = in; \
        T* outPtr = out; \
 \
        const auto KReg = xsimd::batch<T, simdSize>(K); \
 \
        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex) \
        { \
            auto inReg = xsimd::load_unaligned(inPtr); \
            auto outReg = inReg op KReg; \
            outReg.store_unaligned(outPtr); \
 \
            inPtr += simdSize; \
            outPtr += simdSize; \
        } \
 \
        for (size_t elem = (simdSize * numSIMDFrames); elem < len; ++elem) \
        { \
            out[elem] = in[elem] op K; \
        } \
    }

#define K_BY_X_FUNC(func,op) \
    template <typename T> \
    static EnableIfNotComplex<T> func(const T* in, const T& K, T* out, size_t len) \
    { \
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size; \
        const auto numSIMDFrames = len / simdSize; \
 \
        const T* inPtr = in; \
        T* outPtr = out; \
 \
        const auto KReg = xsimd::batch<T, simdSize>(K); \
 \
        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex) \
        { \
            auto inReg = xsimd::load_unaligned(inPtr); \
            auto outReg = KReg op inReg; \
            outReg.store_unaligned(outPtr); \
 \
            inPtr += simdSize; \
            outPtr += simdSize; \
        } \
 \
        for (size_t elem = (simdSize * numSIMDFrames); elem < len; ++elem) \
        { \
            out[elem] = K op in[elem]; \
        } \
    }

X_BY_K_FUNC(XPlusK, +)
X_BY_K_FUNC(XMinusK, -)
K_BY_X_FUNC(KMinusX, -)
X_BY_K_FUNC(XMultK, *)
X_BY_K_FUNC(XDivK, /)
K_BY_X_FUNC(KDivX, /)

}

#define DEFINE_FUNC(func) \
    template <typename T> \
    void func(const T* in, const T& constant, T* out, size_t len) \
    { \
        detail::func(in, constant, out, len); \
    }

DEFINE_FUNC(XPlusK)
DEFINE_FUNC(XMinusK)
DEFINE_FUNC(KMinusX)
DEFINE_FUNC(XMultK)
DEFINE_FUNC(XDivK)
DEFINE_FUNC(KDivX)

#define SPECIALIZE_FUNCS(T) \
    template void XPlusK(const T*, const T&, T*, size_t); \
    template void XMinusK(const T*, const T&, T*, size_t); \
    template void KMinusX(const T*, const T&, T*, size_t); \
    template void XMultK(const T*, const T&, T*, size_t); \
    template void XDivK(const T*, const T&, T*, size_t); \
    template void KDivX(const T*, const T&, T*, size_t);

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
