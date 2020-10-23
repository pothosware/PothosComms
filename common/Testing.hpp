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

    // Copy the input vector some number of times and return a longer vector. This
    // is used to make sure when SIMD implementations are used, the test data is
    // long enough that the SIMD codepaths are tested.
    template <typename T>
    static std::vector<T> stretchStdVector(
        const std::vector<T>& inputs,
        size_t numRepetitions)
    {
        std::vector<T> outputs;
        outputs.reserve(inputs.size() * numRepetitions);

        for(size_t i = 0; i < numRepetitions; ++i)
        {
            outputs.insert(outputs.end(), inputs.begin(), inputs.end());
        }

        return outputs;
    }

    template <typename T>
    static inline Pothos::BufferChunk stdVectorToStretchedBufferChunk(
        const std::vector<T>& inputs,
        size_t numRepetitions)
    {
        return stdVectorToBufferChunk<T>(stretchStdVector<T>(inputs, numRepetitions));
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
