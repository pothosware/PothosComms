// Copyright (c) 2021 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "common/Testing.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>
#include <Pothos/Proxy.hpp>

#include <Poco/Random.h>

#include <cmath>
#include <iostream>
#include <random>

constexpr size_t bufferLen = 150; // Long enough for any SIMD frame, plus some manual operations

static Poco::Random rng;

template <typename Type>
static void getTestValues(
    Pothos::BufferChunk* pInputs,
    Pothos::BufferChunk* pExpectedIntegralOutputs,
    Pothos::BufferChunk* pExpectedFractionalOutputs)
{
    (*pInputs) = Pothos::BufferChunk(typeid(Type), bufferLen);
    (*pExpectedIntegralOutputs) = Pothos::BufferChunk(typeid(Type), bufferLen);
    (*pExpectedFractionalOutputs) = Pothos::BufferChunk(typeid(Type), bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        // nextFloat() returns a value between 0-1
        pInputs->as<Type*>()[elem] = Type(rng.nextFloat() * rng.next(1000000));
        pExpectedFractionalOutputs->as<Type*>()[elem] = std::modf(
            pInputs->as<const Type*>()[elem],
            &pExpectedIntegralOutputs->as<Type*>()[elem]);
    }
}

template <typename Type>
static void testModF()
{
    Pothos::BufferChunk inputs;
    Pothos::BufferChunk expectedIntegralOutputs;
    Pothos::BufferChunk expectedFractionalOutputs;
    getTestValues<Type>(
        &inputs,
        &expectedIntegralOutputs,
        &expectedFractionalOutputs);

    const auto dtype = Pothos::DType(typeid(Type));
    std::cout << "Testing " << dtype.toString() << "..." << std::endl;

    auto source = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto modf = Pothos::BlockRegistry::make("/comms/modf", dtype);
    auto integralSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    auto fractionalSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    source.call("feedBuffer", inputs);

    {
        Pothos::Topology topology;

        topology.connect(source, 0, modf, 0);
        topology.connect(modf, "int", integralSink, 0);
        topology.connect(modf, "frac", fractionalSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    CommsTests::testBufferChunksEqual<Type>(
        expectedIntegralOutputs,
        integralSink.call<Pothos::BufferChunk>("getBuffer"));
    CommsTests::testBufferChunksEqual<Type>(
        expectedFractionalOutputs,
        fractionalSink.call<Pothos::BufferChunk>("getBuffer"));
}

POTHOS_TEST_BLOCK("/comms/tests", test_modf)
{
    testModF<float>();
    testModF<double>();
}
