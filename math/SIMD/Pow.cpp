// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <xsimd/xsimd.hpp>
#include <Pothos/Util/XSIMDTraits.hpp>

#include <cmath>
#include <complex>
#include <type_traits>

// Actually enforce EnableForSIMDPow
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
    struct IsSIMDPowSupported: std::integral_constant<bool,
        Pothos::Util::XSIMDTraits<T>::IsSupported &&
        !IsComplex<T>::value> {};

    template <typename T, typename Ret>
    using EnableForSIMDPow = typename std::enable_if<IsSIMDPowSupported<T>::value, Ret>::type;

    template <typename T, typename Ret>
    using EnableForDefaultPow = typename std::enable_if<!IsSIMDPowSupported<T>::value, Ret>::type;

    template <typename T>
    static void powUnoptimized(const T* in, T* out, T exponent, size_t len)
    {
        for (size_t elem = 0; elem < len; ++elem)
        {
            out[elem] = std::pow(in[elem], exponent);
        }
    }

    template <typename T>
    static EnableForSIMDPow<T, void> pow(const T* in, T* out, T exponent, size_t len)
    {
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size;
        const auto numSIMDFrames = len / simdSize;

        const T* inPtr = in;
        T* outPtr = out;
        const auto exponentReg = xsimd::batch<T, simdSize>(exponent);

        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex)
        {
            auto inReg = xsimd::load_unaligned(inPtr);
            auto outReg = xsimd::pow(inReg, exponentReg);
            outReg.store_unaligned(outPtr);

            inPtr += simdSize;
            outPtr += simdSize;
        }

        powUnoptimized(inPtr, outPtr, exponent, (len - (inPtr - in)));
    }

    template <typename T>
    static EnableForDefaultPow<T, void> pow(const T* in, T* out, T exponent, size_t len)
    {
        powUnoptimized(in, out, exponent, len);
    }
}

// Hide the SFINAE
template <typename T>
void pow(const T* in, T* out, T exponent, size_t len)
{
    detail::pow(in, out, exponent, len);
}

#define POW(T) template void pow(const T*, T*, T, size_t);

    POW(float)
    POW(double)

}}
