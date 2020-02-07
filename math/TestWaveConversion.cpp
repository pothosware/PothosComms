// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>

#include <complex>
#include <cstring>
#include <iostream>

//
// Common test code
//

// Test two arrays for near-equality
#if !defined(POTHOS_TEST_CLOSEA)
    #define POTHOS_TEST_CLOSEA(lhs, rhs, tol, size) \
    { \
        for (size_t i = 0; i < (size); i++) \
        { \
            __POTHOS_TEST_ASSERT( \
                "index " + Pothos::TestingBase::current().toString(i) + \
                " asserts " + Pothos::TestingBase::current().toString((lhs)[i]) + \
                " == " + Pothos::TestingBase::current().toString((rhs)[i]), (std::abs((lhs)[i] - (rhs)[i]) <= (tol))); \
        } \
    }
#endif

static constexpr double tolerance = 1e-6;

static void setAndCheckUnit(
    const Pothos::Proxy& block,
    const std::string& unit)
{
    block.call("setUnit", unit);
    POTHOS_TEST_EQUAL(unit, block.call<std::string>("getUnit"));

    std::cout << " * " << unit << std::endl;
}

template <typename T>
static void testBlock(
    const Pothos::Proxy& block,
    const std::vector<std::complex<T>>& inputs,
    const std::vector<T>& expectedOutputs)
{
    static const Pothos::DType dtype(typeid(T));
    static const Pothos::DType complexDType(typeid(std::complex<T>));

    Pothos::BufferChunk inputBuff(complexDType, inputs.size());
    std::memcpy(
        reinterpret_cast<void*>(inputBuff.address),
        inputs.data(),
        inputs.size() * sizeof(std::complex<T>));

    auto feederSource = Pothos::BlockRegistry::make(
                            "/blocks/feeder_source",
                            complexDType);
    feederSource.call("feedBuffer", inputBuff);

    auto collectorSink = Pothos::BlockRegistry::make(
                             "/blocks/collector_sink",
                             dtype);

    // Execute the topology.
    {
        Pothos::Topology topology;
        topology.connect(feederSource, 0, block, 0);
        topology.connect(block, 0, collectorSink, 0);
        topology.commit();

        POTHOS_TEST_TRUE(topology.waitInactive(0.05));
    }

    Pothos::BufferChunk outputBuff = collectorSink.call("getBuffer");
    POTHOS_TEST_EQUAL(
        dtype.name(),
        outputBuff.dtype.name());
    POTHOS_TEST_EQUAL(
        inputBuff.elements(),
        outputBuff.elements());
    POTHOS_TEST_CLOSEA(
        expectedOutputs.data(),
        outputBuff.as<const T*>(),
        tolerance,
        expectedOutputs.size());
}

//
// Tests
//

template <typename T>
static void testdB(const Pothos::Proxy& block)
{
    setAndCheckUnit(block, "dB");

    const std::vector<std::complex<T>> inputs = {{0.6,0.8}, {0.12345,0.6789}};
    const std::vector<T> expectedOutputs = {0.0, -3.2226066922564565};

    testBlock(block, inputs, expectedOutputs);
}

template <typename T>
static void testdBm(const Pothos::Proxy& block)
{
    setAndCheckUnit(block, "dBm");

    const std::vector<std::complex<T>> inputs = {{0.6,0.8}, {0.12345,0.6789}};
    const std::vector<T> expectedOutputs = {0.0, -1.6113033461282282}; 

    testBlock(block, inputs, expectedOutputs);
}

template <typename T>
static void testWaveConversion()
{
    const Pothos::DType dtype(typeid(T));
    const Pothos::DType complexDType(typeid(std::complex<T>));

    std::cout << "Testing " << complexDType.name() << " -> " << dtype.name() << "..." << std::endl;

    auto waveConversion = Pothos::BlockRegistry::make(
                              "/comms/wave_conversion",
                              dtype);

    // Check default unit.
    POTHOS_TEST_EQUAL("dB", waveConversion.call<std::string>("getUnit"));

    testdB<T>(waveConversion);
    testdBm<T>(waveConversion);
}

POTHOS_TEST_BLOCK("/comms/tests", test_wave_conversion)
{
    testWaveConversion<float>();
    testWaveConversion<double>();
}
