// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-Clause-3

#include <xsimd/xsimd.hpp>
#include <Pothos/Util/XSIMDTraits.hpp>

#include <complex>
#include <type_traits>

// Actually enforce EnableForSIMDConstComparator
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

    template <typename T, typename Ret>
    using EnableForSIMDConstArithmetic = typename std::enable_if<
                                                      Pothos::Util::XSIMDTraits<T>::IsSupported && !IsComplex<T>::value,
                                                      Ret
                                                  >::type;

    template <typename T, typename Ret>
    using EnableForDefaultConstArithmetic = typename std::enable_if<
                                                         !Pothos::Util::XSIMDTraits<T>::IsSupported || IsComplex<T>::value,
                                                         Ret
                                                     >::type;

#define X_BY_K_FUNC(func,op) \
    template <typename T> \
    static void func ## Unoptimized(const T* in, const T& K, T* out, size_t len) \
    { \
        for (size_t elem = 0; elem < len; ++elem) \
        { \
            out[elem] = in[elem] op K; \
        } \
    } \
 \
    template <typename T> \
    static EnableForSIMDConstArithmetic<T, void> func(const T* in, const T& K, T* out, size_t len) \
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
        func ## Unoptimized(inPtr, K, outPtr, (len - (inPtr - in))); \
    } \
 \
    template <typename T> \
    static EnableForDefaultConstArithmetic<T, void> func(const T* in, const T& K, T* out, size_t len) \
    { \
        func ## Unoptimized(in, K, out, len); \
    }

#define K_BY_X_FUNC(func,op) \
    template <typename T> \
    static void func ## Unoptimized(const T* in, const T& K, T* out, size_t len) \
    { \
        for (size_t elem = 0; elem < len; ++elem) \
        { \
            out[elem] = K op in[elem]; \
        } \
    } \
 \
    template <typename T> \
    static EnableForSIMDConstArithmetic<T, void> func(const T* in, const T& K, T* out, size_t len) \
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
        func ## Unoptimized(inPtr, K, outPtr, (len - (inPtr - in))); \
    } \
 \
    template <typename T> \
    static EnableForDefaultConstArithmetic<T, void> func(const T* in, const T& K, T* out, size_t len) \
    { \
        func ## Unoptimized(in, K, out, len); \
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

#define SPECIALIZE_FUNCS_(T) \
    template void XPlusK(const T*, const T&, T*, size_t); \
    template void XMinusK(const T*, const T&, T*, size_t); \
    template void KMinusX(const T*, const T&, T*, size_t); \
    template void XMultK(const T*, const T&, T*, size_t); \
    template void XDivK(const T*, const T&, T*, size_t); \
    template void KDivX(const T*, const T&, T*, size_t);

#define SPECIALIZE_FUNCS(T) \
    SPECIALIZE_FUNCS_(T) \
    SPECIALIZE_FUNCS_(std::complex<T>)

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
