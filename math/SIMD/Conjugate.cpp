// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <xsimd/xsimd.hpp>

#include <cmath>
#include <complex>
#include <type_traits>

// Actually enforce EnableConjSIMD
#ifdef _MSC_VER
#pragma warning(error: 4667) // no function template defined that matches forced instantiation
#endif

template <typename T>
struct IsComplex : std::false_type {};

template <typename T>
struct IsComplex<std::complex<T>> : std::true_type {};

// Only support complex types.
template <typename T>
using EnableConjSIMD = typename std::enable_if<IsComplex<T>::value>::type;

#if !defined POTHOS_SIMD_NAMESPACE
#error Must define POTHOS_SIMD_NAMESPACE to build this file
#endif

namespace PothosCommsSIMD { namespace POTHOS_SIMD_NAMESPACE {

    // TODO: explain getImagMask()

    template <typename T>
    static inline xsimd::batch_bool<T, 2> getImagMask(const xsimd::batch<T, 2>*)
    {
        return xsimd::batch_bool<T, 2>(false, true);
    }

    template <typename T>
    static inline xsimd::batch_bool<T, 4> getImagMask(const xsimd::batch<T, 4>*)
    {
        return xsimd::batch_bool<T, 4>(false, true, false, true);
    }

    template <typename T>
    static inline xsimd::batch_bool<T, 8> getImagMask(const xsimd::batch<T, 8>*)
    {
        return xsimd::batch_bool<T, 8>(
            false, true, false, true, false, true, false, true
            );
    }

    template <typename T>
    static inline xsimd::batch_bool<T, 16> getImagMask(const xsimd::batch<T, 16>*)
    {
        return xsimd::batch_bool<T, 16>(
            false, true, false, true, false, true, false, true,
            false, true, false, true, false, true, false, true
            );
    }

    template <typename T>
    static inline xsimd::batch_bool<T, 32> getImagMask(const xsimd::batch<T, 32>*)
    {
        return xsimd::batch_bool<T, 32>(
            false, true, false, true, false, true, false, true,
            false, true, false, true, false, true, false, true,
            false, true, false, true, false, true, false, true,
            false, true, false, true, false, true, false, true
            );
    }

    template <typename T>
    static inline xsimd::batch_bool<T, 64> getImagMask(const xsimd::batch<T, 64>*)
    {
        return xsimd::batch_bool<T, 64>(
            false, true, false, true, false, true, false, true,
            false, true, false, true, false, true, false, true,
            false, true, false, true, false, true, false, true,
            false, true, false, true, false, true, false, true,
            false, true, false, true, false, true, false, true,
            false, true, false, true, false, true, false, true,
            false, true, false, true, false, true, false, true,
            false, true, false, true, false, true, false, true
            );
    }

    template <typename T>
    EnableConjSIMD<T> conj(const T* in, T* out, size_t len)
    {
        using ScalarType = typename T::value_type;
        static constexpr size_t simdSize = xsimd::simd_traits<ScalarType>::size;

        const auto scalarLen = len * 2;
        const auto numSIMDFrames = scalarLen / simdSize;

        static const xsimd::batch<ScalarType, simdSize>* batchPtrParam = nullptr;
        static const auto imagMask = getImagMask<ScalarType>(batchPtrParam);

        const ScalarType* scalarInPtr = (const ScalarType*)in;
        ScalarType* scalarOutPtr = (ScalarType*)out;

        static const auto NegOneReg = xsimd::batch<ScalarType, simdSize>(-1);

        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex)
        {
            auto inReg = xsimd::load_unaligned(scalarInPtr);

            auto outReg = xsimd::select(imagMask, (inReg * NegOneReg), inReg);
            outReg.store_unaligned(scalarOutPtr);

            scalarInPtr += simdSize;
            scalarOutPtr += simdSize;
        }

        for (size_t elem = (simdSize * numSIMDFrames / 2); elem < len; ++elem)
        {
            out[elem] = std::conj(in[elem]);
        }
    }

#define CONJ(T) template void conj(const std::complex<T>*, std::complex<T>*, size_t);

    CONJ(std::int8_t)
    // Due to an XSIMD limitation, no int16_t implementation
    CONJ(std::int32_t)
    CONJ(std::int64_t)
    CONJ(float)
    CONJ(double)

}}
