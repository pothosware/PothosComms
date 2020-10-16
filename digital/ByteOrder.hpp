// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <Poco/ByteOrder.h>
#include <Poco/Platform.h>

#include <complex>
#include <type_traits>

#if POCO_OS == POCO_OS_MAC_OS_X
#include <libkern/OSByteOrder.h>
#endif

namespace detail
{
    template <typename T>
    struct IsComplex : std::false_type {};

    template <typename T>
    struct IsComplex<std::complex<T>> : std::true_type {};

    template <typename T>
    struct BufferType
    {
        using Type = T;
    };

    template <>
    struct BufferType<Poco::Int16>
    {
        using Type = Poco::UInt16;
    };

    template <>
    struct BufferType<Poco::Int32>
    {
        using Type = Poco::UInt32;
    };

    template <>
    struct BufferType<Poco::Int64>
    {
        using Type = Poco::UInt64;
    };

    template <>
    struct BufferType<float>
    {
        using Type = Poco::UInt32;
    };

    template <>
    struct BufferType<double>
    {
        using Type = Poco::UInt64;
    };

    template <typename T>
    struct NoReinterpretCast : std::integral_constant<bool,
        std::is_same<T, typename BufferType<T>::Type>::value &&
        !IsComplex<T>::value> {};

    template <typename T>
    struct MustReinterpretCast : std::integral_constant<bool,
        !std::is_same<T, typename BufferType<T>::Type>::value &&
        !IsComplex<T>::value> {};

    // For some reason, despite specifically supporting OS X, Poco uses manual
    // bit-shifting instead of OS X's optimized byteswap functions.
#if POCO_OS == POCO_OS_MAC_OS_X
    static inline Poco::UInt16 byteswap(Poco::UInt16 val)
    {
        return OSSwapInt16(val);
    }

    static inline Poco::UInt32 byteswap(Poco::UInt32 val)
    {
        return OSSwapInt32(val);
    }

    static inline Poco::UInt64 byteswap(Poco::UInt64 val)
    {
        return OSSwapInt64(val);
    }
#else
    template <typename T>
    static inline T byteswap(T val)
    {
        return Poco::ByteOrder::flipBytes(val);
    }
#endif
    template <typename T>
    static typename std::enable_if<NoReinterpretCast<T>::value, void>::type byteswapBuffer(const T* in, T* out, size_t numElements)
    {
        for (size_t elem = 0; elem < numElements; ++elem) out[elem] = byteswap(in[elem]);
    }

    template <typename T>
    static typename std::enable_if<MustReinterpretCast<T>::value, void>::type byteswapBuffer(const T* in, T* out, size_t numElements)
    {
        using BufferType = typename BufferType<T>::Type;
        static_assert(sizeof(T) == sizeof(BufferType), "type size mismatch");

        byteswapBuffer<BufferType>((const BufferType*)in, (BufferType*)out, numElements);
    }

    template <typename T>
    static typename std::enable_if<IsComplex<T>::value, void>::type byteswapBuffer(const T* in, T* out, size_t numElements)
    {
        using ScalarType = typename T::value_type;
        byteswapBuffer<ScalarType>((const ScalarType*)in, (ScalarType*)out, (numElements * 2));
    }
}

template <typename T>
static void byteswapBuffer(const T* in, T* out, size_t numElements)
{
    detail::byteswapBuffer(in, out, numElements);
}