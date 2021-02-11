// Copyright (c) 2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <xsimd/xsimd.hpp>
#include <Pothos/Util/XSIMDTraits.hpp>

#include <cmath>
#include <type_traits>

// Actually enforce EnableIf*
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
    static void sigmoidUnoptimized(const T* in, T* out, size_t len)
    {
        for (size_t elem = 0; elem < len; ++elem)
        {
            out[elem] = T(1.0) / (T(1.0) + std::exp(-in[elem]));
        }
    }

    template <typename T>
    static Pothos::Util::EnableIfXSIMDSupports<T, void> sigmoid(const T* in, T* out, size_t len)
    {
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size;
        const auto numSIMDFrames = len / simdSize;

        const T* inPtr = in;
        T* outPtr = out;

        static const auto OneReg = xsimd::batch<T, simdSize>(T(1));

        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex)
        {
            auto inReg = xsimd::load_unaligned(inPtr);

            auto outReg = OneReg / (OneReg + xsimd::exp(-inReg));
            outReg.store_unaligned(outPtr);

            inPtr += simdSize;
            outPtr += simdSize;
        }

        sigmoidUnoptimized(inPtr, outPtr, (len - (inPtr - in)));
    }

    template <typename T>
    static inline Pothos::Util::EnableIfXSIMDDoesNotSupport<T, void> sigmoid(const T* in, T* out, size_t len)
    {
        sigmoidUnoptimized(in, out, len);
    }
}

template <typename T>
void sigmoid(const T* in, T* out, size_t len)
{
    detail::sigmoid(in, out, len);
}

#define SIGMOID(T) template void sigmoid(const T*, T*, size_t);

    SIGMOID(float)
    SIGMOID(double)

}}
