// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

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
#define LOG_FUNC(func) \
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
    static Pothos::Util::EnableIfXSIMDSupports<T, void> func(const T* in, T* out, size_t len) \
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
    static inline Pothos::Util::EnableIfXSIMDDoesNotSupport<T, void> func(const T* in, T* out, size_t len) \
    { \
        func ## Unoptimized(in, out, len); \
    }

    LOG_FUNC(log)
    LOG_FUNC(log2)
    LOG_FUNC(log10)
    LOG_FUNC(log1p)

    template <typename T>
    static void logNUnoptimized(const T* in, T* out, T base, size_t len)
    {
        for (size_t elem = 0; elem < len; ++elem)
        {
            out[elem] = std::log(in[elem]) / std::log(base);
        }
    }

    template <typename T>
    static Pothos::Util::EnableIfXSIMDSupports<T, void> logN(const T* in, T* out, T base, size_t len)
    {
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size;
        const auto numSIMDFrames = len / simdSize;

        const T* inPtr = in;
        T* outPtr = out;

        const auto logBaseReg = xsimd::log(xsimd::batch<T, simdSize>(base));

        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex)
        {
            auto inReg = xsimd::load_unaligned(inPtr);
            auto outReg = xsimd::log(inReg) / logBaseReg;
            outReg.store_unaligned(outPtr);

            inPtr += simdSize;
            outPtr += simdSize;
        }

        // Perform the remaining operations manually.
        logNUnoptimized(inPtr, outPtr, base, (len - (inPtr - in)));
    }

    template <typename T>
    static Pothos::Util::EnableIfXSIMDDoesNotSupport<T, void> logN(const T* in, T* out, T base, size_t len)
    {
        logNUnoptimized(in, out, base, len);
    }
}

// Hide the SFINAE
#define DEFINE_FUNC(func) \
    template <typename T> \
    void func(const T* in, T* out, size_t num) \
    { \
        detail::func(in, out, num); \
    }

DEFINE_FUNC(log)
DEFINE_FUNC(log2)
DEFINE_FUNC(log10)
DEFINE_FUNC(log1p)

// Different signature
template <typename T>
void logN(const T* in, T* out, T base, size_t num)
{
    detail::logN(in, out, base, num);
}

#define SPECIALIZE_FUNCS(T) \
    template void log(const T*, T*, size_t); \
    template void log2(const T*, T*, size_t); \
    template void log10(const T*, T*, size_t); \
    template void log1p(const T*, T*, size_t); \
    template void logN(const T*, T*, T, size_t); \

    SPECIALIZE_FUNCS(float)
    SPECIALIZE_FUNCS(double)

}}
