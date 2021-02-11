// Copyright (c) 2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <xsimd/xsimd.hpp>
#include <Pothos/Util/XSIMDTraits.hpp>

#include <cmath>
#include <complex>
#include <type_traits>

// Actually enforce SupportSIMDModF
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
    struct SupportSIMDModF: std::integral_constant<bool,
                                std::is_floating_point<T>::value &&
                                Pothos::Util::XSIMDTraits<T>::IsSupported
                            >{};

    template <typename T>
    static void modfUnoptimized(const T* in, T* integralOut, T* fractionalOut, size_t len)
    {
        for (size_t i = 0; i < len; i++)
        {
            fractionalOut[i] = std::modf(in[i], &integralOut[i]);
        }
    }

    template <typename T>
    static typename std::enable_if<SupportSIMDModF<T>::value, void>::type
    modf(const T* in, T* integralOut, T* fractionalOut, size_t len)
    {
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size;
        const auto numSIMDFrames = len / simdSize;

        const T* inPtr = in;
        T* integralOutPtr = integralOut;
        T* fractionalOutPtr = fractionalOut;

        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex)
        {
            auto inReg = xsimd::load_unaligned(inPtr);
            auto integralOutReg = xsimd::trunc(inReg);
            auto fractionalOutReg = inReg - integralOutReg;

            integralOutReg.store_unaligned(integralOutPtr);
            fractionalOutReg.store_unaligned(fractionalOutPtr);

            inPtr += simdSize;
            integralOutPtr += simdSize;
            fractionalOutPtr += simdSize;
        }

        // Perform remaining operations manually.
        modfUnoptimized(inPtr, integralOutPtr, fractionalOutPtr, (len - (inPtr - in)));
    }

    template <typename T>
    static typename std::enable_if<!SupportSIMDModF<T>::value, void>::type
    modf(const T* in, T* integralOut, T* fractionalOut, size_t len)
    {
        modfUnoptimized(in, integralOut, fractionalOut, len);
    }
}

// Hide the SFINAE
template <typename T>
void modf(const T* in, T* integralOut, T* fractionalOut, size_t len)
{
    detail::modf(in, integralOut, fractionalOut, len);
}

#define MODF(T) template void modf(const T*, T*, T*, size_t);

    MODF(float)
    MODF(double)

}}
