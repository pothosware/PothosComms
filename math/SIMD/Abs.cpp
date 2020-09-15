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

// Only support non-complex types.
template <typename T>
using EnableAbsSIMD = typename std::enable_if<!IsComplex<T>::value>::type;

#if !defined POTHOS_SIMD_NAMESPACE
#error Must define POTHOS_SIMD_NAMESPACE to build this file
#endif

namespace PothosCommsSIMD { namespace POTHOS_SIMD_NAMESPACE {

    template <typename T>
    EnableAbsSIMD<T> abs(const T* in, T* out, size_t len)
    {
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size;
        const auto numSIMDFrames = len / simdSize;

        const T* inPtr = in;
        T* outPtr = out;

        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex)
        {
            auto inReg = xsimd::load_unaligned(inPtr);
            auto outReg = xsimd::abs(inReg);
            outReg.store_unaligned(outPtr);

            inPtr += simdSize;
            outPtr += simdSize;
        }

        for (size_t elem = (simdSize * numSIMDFrames); elem < len; ++elem)
        {
            out[elem] = std::abs(in[elem]);
        }
    }

#define ABS(T) template void abs(const T*, T*, size_t);

    ABS(std::int8_t)
    ABS(std::int16_t)
    ABS(std::int32_t)
    ABS(std::int64_t)
    ABS(float)
    ABS(double)

}}