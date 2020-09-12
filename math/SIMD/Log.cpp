// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <xsimd/xsimd.hpp>

#include <cmath>
#include <complex>
#include <type_traits>

template <typename T>
struct IsComplex : std::false_type {};

template <typename T>
struct IsComplex<std::complex<T>> : std::true_type {};

// Only support scalar floating types.
template <typename T>
using EnableLogSIMD = typename std::enable_if<!IsComplex<T>::value && std::is_floating_point<T>::value>::type;

#if !defined POTHOS_SIMD_NAMESPACE
#error Must define POTHOS_SIMD_NAMESPACE to build this file
#endif

namespace PothosCommsSIMD { namespace POTHOS_SIMD_NAMESPACE {

#define LOG_FUNC(func) \
    template <typename T> \
    EnableLogSIMD<T> func(const T* in, T* out, size_t len) \
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
        for (size_t elem = (simdSize * numSIMDFrames); elem < len; ++elem) \
        { \
            out[elem] = std::func(in[elem]); \
        } \
    }

    LOG_FUNC(log)
    LOG_FUNC(log2)
    LOG_FUNC(log10)

    template <typename T>
    EnableLogSIMD<T> logN(const T* in, T* out, T base, size_t len)
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

        for (size_t elem = (simdSize * numSIMDFrames); elem < len; ++elem)
        {
            out[elem] = std::log(in[elem]) / std::log(base);
        }
    }

#define DECLARE_FUNCS(T) \
    template void log(const T*, T*, size_t); \
    template void log2(const T*, T*, size_t); \
    template void log10(const T*, T*, size_t); \
    template void logN(const T*, T*, T, size_t); \

    DECLARE_FUNCS(float)
    DECLARE_FUNCS(double)

}}