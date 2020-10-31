// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "common/Testing.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <cmath>
#include <iostream>
#include <random>

static constexpr size_t BufferLen = 4096;

template <typename T>
struct TestParams
{
    Pothos::BufferChunk inputs;
    Pothos::BufferChunk expectedOutputs;
};

static std::random_device rd;
static std::mt19937 g(rd());

template <typename T>
static TestParams<T> getTestParams()
{
    std::uniform_real_distribution<T> distribution(T(1.0), T(1000.0));

    const Pothos::DType dtype(typeid(T));

    TestParams<T> testParams;
    testParams.inputs = Pothos::BufferChunk(dtype, BufferLen);
    testParams.expectedOutputs = Pothos::BufferChunk(dtype, BufferLen);

    for(size_t elem = 0; elem < BufferLen; ++elem)
    {
        const auto value = distribution(g);

        testParams.inputs.template as<T*>()[elem] = value;
        testParams.expectedOutputs.template as<T*>()[elem] = T(1.0) / std::sqrt(value);
    }

    return testParams;
}

template <typename T>
static void testRSqrt()
{
    const Pothos::DType dtype(typeid(T));

    std::cout << " * Testing " << dtype.name() << "..." << std::endl;

    auto params = getTestParams<T>();

    auto source = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto rsqrt = Pothos::BlockRegistry::make("/comms/rsqrt", dtype);
    auto sink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    source.call("feedBuffer", params.inputs);

    {
        Pothos::Topology topology;
        topology.connect(source, 0, rsqrt, 0);
        topology.connect(rsqrt, 0, sink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    // Increased tolerance due to cascading errors.
    CommsTests::testBufferChunksClose<T>(
        params.expectedOutputs,
        sink.template call<Pothos::BufferChunk>("getBuffer"),
        T(0.1));
}

POTHOS_TEST_BLOCK("/comms/tests", test_rsqrt)
{
    testRSqrt<float>();
    testRSqrt<double>();
}
