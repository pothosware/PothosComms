// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include <cmath>
#include <complex>
#include <iostream>
#include <type_traits>

//
// Utility
//

constexpr size_t bufferLen = 100; // Long enough for any SIMD frame, plus manually adding entries

template <typename T>
struct IsComplex : std::false_type {};

template <typename T>
struct IsComplex<std::complex<T>> : std::true_type {};

template <typename T>
static typename std::enable_if<!IsComplex<T>::value, Pothos::BufferChunk>::type getTestInputs()
{
    static const Pothos::DType dtype(typeid(T));

    Pothos::BufferChunk bufferChunk(dtype, bufferLen);
    for (size_t i = 0; i < bufferLen; ++i)
    {
        bufferChunk.as<T*>()[i] = T(i) - T(bufferLen/2);
    }

    return bufferChunk;
}

template <typename T>
static typename std::enable_if<IsComplex<T>::value, Pothos::BufferChunk>::type getTestInputs()
{
    using ScalarType = typename T::value_type;

    static const Pothos::DType dtype(typeid(T));

    auto bufferChunk = getTestInputs<ScalarType>();
    bufferChunk.dtype = dtype;

    return bufferChunk;
}

template <typename InType, typename OutType>
struct AbsTestValues
{
    Pothos::BufferChunk input;
    Pothos::BufferChunk expectedOutput;

    AbsTestValues()
    {
        input = getTestInputs<InType>();
        expectedOutput = Pothos::BufferChunk(typeid(OutType), input.elements());

        const InType* inPtr = input;
        OutType* outPtr = expectedOutput;

        for (size_t elem = 0; elem < input.elements(); ++elem)
        {
            outPtr[elem] = std::abs(inPtr[elem]);
        }
    }
};

template <typename InType, typename OutType>
static void testAbs()
{
    const auto inDType = Pothos::DType(typeid(InType));
    const auto outDType = Pothos::DType(typeid(OutType));

    std::cout << "Testing " << inDType.toString() << "..." << std::endl;

    AbsTestValues<InType, OutType> testValues;

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", inDType);
    feeder.call("feedBuffer", testValues.input);

    auto abs = Pothos::BlockRegistry::make("/comms/abs", inDType);
    auto sink = Pothos::BlockRegistry::make("/blocks/collector_sink", outDType);

    {
        Pothos::Topology topology;

        topology.connect(feeder, 0, abs, 0);
        topology.connect(abs, 0, sink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    auto output = sink.call<Pothos::BufferChunk>("getBuffer");
    POTHOS_TEST_EQUAL(testValues.expectedOutput.dtype, output.dtype);
    POTHOS_TEST_EQUAL(testValues.expectedOutput.elements(), output.elements());
    POTHOS_TEST_EQUALA(
        testValues.expectedOutput.as<const OutType*>(),
        output.as<const OutType*>(),
        testValues.expectedOutput.elements());
}

POTHOS_TEST_BLOCK("/comms/tests", test_abs)
{
    testAbs<std::int8_t, std::int8_t>();
    testAbs<std::int16_t, std::int16_t>();
    testAbs<std::int32_t, std::int32_t>();
    testAbs<std::int64_t, std::int64_t>();
    testAbs<float, float>();
    testAbs<double, double>();
    testAbs<std::complex<std::int8_t>, std::int8_t>();
    testAbs<std::complex<std::int16_t>, std::int16_t>();
    testAbs<std::complex<std::int32_t>, std::int32_t>();
    testAbs<std::complex<std::int64_t>, std::int64_t>();
    testAbs<std::complex<float>, float>();
    testAbs<std::complex<double>, double>();
}