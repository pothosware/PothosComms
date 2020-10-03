// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>
#include <Pothos/Proxy.hpp>

#include <complex>
#include <iostream>
#include <random>

//
// Common
//

constexpr size_t bufferLen = 150; // Long enough for any SIMD frame, plus some manual operations

template <typename Type>
static void getTestValues(
    Pothos::BufferChunk* pInputs,
    Pothos::BufferChunk* pExpectedOutputs)
{
    using ComplexType = std::complex<Type>;

    (*pInputs) = Pothos::BufferChunk(typeid(ComplexType), bufferLen);
    (*pExpectedOutputs) = Pothos::BufferChunk(typeid(ComplexType), bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        pInputs->as<ComplexType*>()[elem] = ComplexType(Type(rand() % 100), Type(rand() % 100));
        pExpectedOutputs->as<ComplexType*>()[elem] = std::conj(pInputs->as<ComplexType*>()[elem]);
    }
}

template <typename Type>
static void testConjugate()
{
    using ComplexType = std::complex<Type>;

    Pothos::BufferChunk inputs;
    Pothos::BufferChunk expectedOutputs;
    getTestValues<Type>(&inputs, &expectedOutputs);

    const auto dtype = Pothos::DType(typeid(ComplexType));

    auto source = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto conj = Pothos::BlockRegistry::make("/comms/conjugate", dtype);
    auto sink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    source.call("feedBuffer", inputs);

    {
        Pothos::Topology topology;

        topology.connect(source, 0, conj, 0);
        topology.connect(conj, 0, sink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    auto outputs = sink.call<Pothos::BufferChunk>("getBuffer");
    POTHOS_TEST_EQUAL(expectedOutputs.dtype, outputs.dtype);
    POTHOS_TEST_EQUAL(expectedOutputs.elements(), outputs.elements());
    POTHOS_TEST_EQUALA(
        expectedOutputs.as<const ComplexType*>(),
        outputs.as<const ComplexType*>(),
        bufferLen);
}

POTHOS_TEST_BLOCK("/comms/tests", test_conjugate)
{
    testConjugate<std::int8_t>();
    testConjugate<std::int16_t>();
    testConjugate<std::int32_t>();
    testConjugate<std::int64_t>();
    testConjugate<float>();
    testConjugate<double>();
}