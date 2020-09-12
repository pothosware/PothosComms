// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <xsimd/xsimd.hpp>

#include <cmath>

#if !defined POTHOS_SIMD_NAMESPACE
#error Must define POTHOS_SIMD_NAMESPACE to build this file
#endif

namespace PothosCommsSIMD { namespace POTHOS_SIMD_NAMESPACE {

// For when the function exists in both xsimd:: and std:: without any manual calculation (1/func(x), etc)
#define XSIMD_TRIG_FUNC(func) \
    template <typename T> \
    void func(const T* in, T* out, size_t len) \
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

// func = 1 / baseFunc(x)
#define XSIMD_TRIG_ONEDIV_FUNC(func,baseFunc) \
    template <typename T> \
    void func(const T* in, T* out, size_t len) \
    { \
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size; \
        const auto numSIMDFrames = len / simdSize; \
 \
        const T* inPtr = in; \
        T* outPtr = out; \
 \
        static const auto OneReg = xsimd::batch<T, simdSize>(T(1)); \
 \
        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex) \
        { \
            auto inReg = xsimd::load_unaligned(inPtr); \
            auto outReg = OneReg / xsimd::baseFunc(inReg); \
            outReg.store_unaligned(outPtr); \
 \
            inPtr += simdSize; \
            outPtr += simdSize; \
        } \
 \
        for (size_t elem = (simdSize * numSIMDFrames); elem < len; ++elem) \
        { \
            out[elem] = T(1) / std::baseFunc(in[elem]); \
        } \
    }

// func = baseFunc(1 / x)
#define XSIMD_TRIG_FUNC_ONEDIVX(func,baseFunc) \
    template <typename T> \
    void func(const T* in, T* out, size_t len) \
    { \
        static constexpr size_t simdSize = xsimd::simd_traits<T>::size; \
        const auto numSIMDFrames = len / simdSize; \
 \
        const T* inPtr = in; \
        T* outPtr = out; \
 \
        static const auto OneReg = xsimd::batch<T, simdSize>(T(1)); \
 \
        for (size_t frameIndex = 0; frameIndex < numSIMDFrames; ++frameIndex) \
        { \
            auto inReg = xsimd::load_unaligned(inPtr); \
            auto outReg = xsimd::baseFunc(OneReg / inReg); \
            outReg.store_unaligned(outPtr); \
 \
            inPtr += simdSize; \
            outPtr += simdSize; \
        } \
 \
        for (size_t elem = (simdSize * numSIMDFrames); elem < len; ++elem) \
        { \
            out[elem] = std::baseFunc(T(1) / in[elem]); \
        } \
    }

    XSIMD_TRIG_FUNC(cos)
    XSIMD_TRIG_FUNC(sin)
    XSIMD_TRIG_FUNC(tan)
    XSIMD_TRIG_ONEDIV_FUNC(sec, cos)
    XSIMD_TRIG_ONEDIV_FUNC(csc, sin)
    XSIMD_TRIG_ONEDIV_FUNC(cot, tan)
    XSIMD_TRIG_FUNC(acos)
    XSIMD_TRIG_FUNC(asin)
    XSIMD_TRIG_FUNC(atan)
    XSIMD_TRIG_FUNC_ONEDIVX(asec, acos)
    XSIMD_TRIG_FUNC_ONEDIVX(acsc, asin)
    XSIMD_TRIG_FUNC_ONEDIVX(acot, atan)
    XSIMD_TRIG_FUNC(cosh)
    XSIMD_TRIG_FUNC(sinh)
    XSIMD_TRIG_FUNC(tanh)
    XSIMD_TRIG_ONEDIV_FUNC(sech, cosh)
    XSIMD_TRIG_ONEDIV_FUNC(csch, sinh)
    XSIMD_TRIG_ONEDIV_FUNC(coth, tanh)
    XSIMD_TRIG_FUNC(acosh)
    XSIMD_TRIG_FUNC(asinh)
    XSIMD_TRIG_FUNC(atanh)
    XSIMD_TRIG_FUNC_ONEDIVX(asech, acosh)
    XSIMD_TRIG_FUNC_ONEDIVX(acsch, asinh)
    XSIMD_TRIG_FUNC_ONEDIVX(acoth, atanh)

#define DECLARE_FUNCS(T) \
    template void cos<T>(const T*, T*, size_t); \
    template void sin<T>(const T*, T*, size_t); \
    template void tan<T>(const T*, T*, size_t); \
    template void sec<T>(const T*, T*, size_t); \
    template void csc<T>(const T*, T*, size_t); \
    template void cot<T>(const T*, T*, size_t); \
    template void acos<T>(const T*, T*, size_t); \
    template void asin<T>(const T*, T*, size_t); \
    template void atan<T>(const T*, T*, size_t); \
    template void asec<T>(const T*, T*, size_t); \
    template void acsc<T>(const T*, T*, size_t); \
    template void acot<T>(const T*, T*, size_t); \
    template void cosh<T>(const T*, T*, size_t); \
    template void sinh<T>(const T*, T*, size_t); \
    template void tanh<T>(const T*, T*, size_t); \
    template void sech<T>(const T*, T*, size_t); \
    template void csch<T>(const T*, T*, size_t); \
    template void coth<T>(const T*, T*, size_t); \
    template void acosh<T>(const T*, T*, size_t); \
    template void asinh<T>(const T*, T*, size_t); \
    template void atanh<T>(const T*, T*, size_t); \
    template void asech<T>(const T*, T*, size_t); \
    template void acsch<T>(const T*, T*, size_t); \
    template void acoth<T>(const T*, T*, size_t);

DECLARE_FUNCS(float)
DECLARE_FUNCS(double)

}}