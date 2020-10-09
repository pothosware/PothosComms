// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <complex>
#include <cstring>
#include <type_traits>
#include <vector>

namespace CommsTests
{
    template <typename T>
    struct IsComplex : std::false_type {};

    template <typename T>
    struct IsComplex<std::complex<T>> : std::true_type {};

    template <typename T, typename Ret>
    using EnableIfComplex = typename std::enable_if<IsComplex<T>::value, Ret>::type;

    template <typename T, typename Ret>
    using DisableIfComplex = typename std::enable_if<!IsComplex<T>::value, Ret>::type;

    template <typename T>
    static Pothos::BufferChunk stdVectorToBufferChunk(const std::vector<T>& inputs)
    {
        Pothos::BufferChunk ret(Pothos::DType(typeid(T)), inputs.size());
        std::memcpy(
            reinterpret_cast<void*>(ret.address),
            inputs.data(),
            ret.length);

        return ret;
    }

    template <typename T>
    static void testBufferChunksEqual(
        const Pothos::BufferChunk& expected,
        const Pothos::BufferChunk& actual)
    {
        POTHOS_TEST_EQUAL(expected.dtype, actual.dtype);
        POTHOS_TEST_EQUAL(expected.elements(), actual.elements());
        POTHOS_TEST_EQUALA(
            expected.as<const T*>(),
            actual.as<const T*>(),
            expected.elements());
    }

    template <typename T>
    static void testBufferChunksClose(
        const Pothos::BufferChunk& expected,
        const Pothos::BufferChunk& actual,
        T epsilon)
    {
        POTHOS_TEST_EQUAL(expected.dtype, actual.dtype);
        POTHOS_TEST_EQUAL(expected.elements(), actual.elements());
        POTHOS_TEST_CLOSEA(
            expected.as<const T*>(),
            actual.as<const T*>(),
            epsilon,
            expected.elements());
    }
}