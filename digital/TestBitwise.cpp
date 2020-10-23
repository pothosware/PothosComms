// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "common/Testing.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <Poco/RandomStream.h>

#include <iostream>
#include <vector>

//
// Utility code
//

// Enough for any SIMD frame, plus some extra manual calculations
static constexpr size_t bufferLen = 150;

template <typename T>
static Pothos::BufferChunk getTestInputs()
{
    Pothos::BufferChunk bufferChunk(typeid(T), bufferLen);
    Poco::RandomBuf randomBuf;
    randomBuf.readFromDevice(bufferChunk, bufferChunk.length);

    return bufferChunk;
}

template <typename T>
static T getRandomValue()
{
    T constant;
    Poco::RandomBuf randomBuf;
    randomBuf.readFromDevice((char*)&constant, sizeof(constant));

    return constant;
}

//
// Test implementations
//

template <typename T>
static void testBitwiseUnaryArray()
{
    const Pothos::DType dtype(typeid(T));

    std::cout << "Testing " << dtype.name() << "..." << std::endl;

    auto input = getTestInputs<T>();
    Pothos::BufferChunk expectedOutput(typeid(T), input.elements());
    for (size_t elem = 0; elem < expectedOutput.elements(); ++elem)
    {
        expectedOutput.template as<T*>()[elem] = ~input.template as<const T*>()[elem];
    }

    auto source = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto notBlock = Pothos::BlockRegistry::make("/comms/bitwise_unary", dtype, "NOT");
    auto sink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    source.call("feedBuffer", input);

    {
        Pothos::Topology topology;

        topology.connect(source, 0, notBlock, 0);
        topology.connect(notBlock, 0, sink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    CommsTests::testBufferChunksEqual<T>(
        expectedOutput,
        sink.call<Pothos::BufferChunk>("getBuffer"));
}

template <typename T>
static void testBitwiseBinaryArray()
{
    const Pothos::DType dtype(typeid(T));
    constexpr size_t numInputs = 3;

    std::cout << "Testing " << dtype.name() << "..." << std::endl;

    std::vector<Pothos::BufferChunk> inputs;
    for (size_t i = 0; i < numInputs; ++i) inputs.emplace_back(getTestInputs<T>());

    Pothos::BufferChunk expectedAndOutput(dtype, bufferLen);
    Pothos::BufferChunk expectedOrOutput(dtype, bufferLen);
    Pothos::BufferChunk expectedXOrOutput(dtype, bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        expectedAndOutput.template as<T*>()[elem] = inputs[0].template as<T*>()[elem] & inputs[1].template as<T*>()[elem] & inputs[2].template as<T*>()[elem];
        expectedOrOutput.template as<T*>()[elem] = inputs[0].template as<T*>()[elem] | inputs[1].template as<T*>()[elem] | inputs[2].template as<T*>()[elem];
        expectedXOrOutput.template as<T*>()[elem] = inputs[0].template as<T*>()[elem] ^ inputs[1].template as<T*>()[elem] ^ inputs[2].template as<T*>()[elem];
    }

    std::vector<Pothos::Proxy> sources(numInputs);
    for (size_t input = 0; input < numInputs; ++input)
    {
        sources[input] = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
        sources[input].call("feedBuffer", inputs[input]);
    }

    auto andBlock = Pothos::BlockRegistry::make("/comms/bitwise_binary", dtype, "AND", numInputs);
    auto andSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    auto orBlock = Pothos::BlockRegistry::make("/comms/bitwise_binary", dtype, "OR", numInputs);
    auto orSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    auto xorBlock = Pothos::BlockRegistry::make("/comms/bitwise_binary", dtype, "XOR", numInputs);
    auto xorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;

        for (size_t input = 0; input < numInputs; ++input)
        {
            topology.connect(sources[input], 0, andBlock, input);
            topology.connect(sources[input], 0, orBlock, input);
            topology.connect(sources[input], 0, xorBlock, input);
        }

        topology.connect(andBlock, 0, andSink, 0);
        topology.connect(orBlock, 0, orSink, 0);
        topology.connect(xorBlock, 0, xorSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    std::cout << " * Testing AND..." << std::endl;
    CommsTests::testBufferChunksEqual<T>(
        expectedAndOutput,
        andSink.call<Pothos::BufferChunk>("getBuffer"));

    std::cout << " * Testing OR..." << std::endl;
    CommsTests::testBufferChunksEqual<T>(
        expectedOrOutput,
        orSink.call<Pothos::BufferChunk>("getBuffer"));

    std::cout << " * Testing XOR..." << std::endl;
    CommsTests::testBufferChunksEqual<T>(
        expectedXOrOutput,
        xorSink.call<Pothos::BufferChunk>("getBuffer"));
}

template <typename T>
static void testBitwiseBinaryConst()
{
    const Pothos::DType dtype(typeid(T));

    std::cout << "Testing " << dtype.name() << "..." << std::endl;

    auto input = getTestInputs<T>();
    const auto constant = getRandomValue<T>();

    Pothos::BufferChunk expectedAndOutput(dtype, bufferLen);
    Pothos::BufferChunk expectedOrOutput(dtype, bufferLen);
    Pothos::BufferChunk expectedXOrOutput(dtype, bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        expectedAndOutput.template as<T*>()[elem] = input.template as<T*>()[elem] & constant;
        expectedOrOutput.template as<T*>()[elem] = input.template as<T*>()[elem] | constant;
        expectedXOrOutput.template as<T*>()[elem] = input.template as<T*>()[elem] ^ constant;
    }

    auto source = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    source.call("feedBuffer", input);

    auto andBlock = Pothos::BlockRegistry::make("/comms/const_bitwise_binary", dtype, constant, "AND");
    auto andSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    POTHOS_TEST_EQUAL(constant, andBlock.template call<T>("constant"));

    auto orBlock = Pothos::BlockRegistry::make("/comms/const_bitwise_binary", dtype, constant, "OR");
    auto orSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    POTHOS_TEST_EQUAL(constant, orBlock.template call<T>("constant"));

    auto xorBlock = Pothos::BlockRegistry::make("/comms/const_bitwise_binary", dtype, constant, "XOR");
    auto xorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    POTHOS_TEST_EQUAL(constant, xorBlock.template call<T>("constant"));

    {
        Pothos::Topology topology;

        topology.connect(source, 0, andBlock, 0);
        topology.connect(source, 0, orBlock, 0);
        topology.connect(source, 0, xorBlock, 0);

        topology.connect(andBlock, 0, andSink, 0);
        topology.connect(orBlock, 0, orSink, 0);
        topology.connect(xorBlock, 0, xorSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    std::cout << " * Testing AND..." << std::endl;
    CommsTests::testBufferChunksEqual<T>(
        expectedAndOutput,
        andSink.call<Pothos::BufferChunk>("getBuffer"));

    std::cout << " * Testing OR..." << std::endl;
    CommsTests::testBufferChunksEqual<T>(
        expectedOrOutput,
        orSink.call<Pothos::BufferChunk>("getBuffer"));

    std::cout << " * Testing XOR..." << std::endl;
    CommsTests::testBufferChunksEqual<T>(
        expectedXOrOutput,
        xorSink.call<Pothos::BufferChunk>("getBuffer"));
}

template <typename T>
static void testBitShift()
{
    const Pothos::DType dtype(typeid(T));

    std::cout << "Testing " << dtype.name() << "..." << std::endl;

    auto input = getTestInputs<T>();
    constexpr size_t leftShiftSize = ((sizeof(T) * 8) / 2);
    constexpr size_t rightShiftSize = ((sizeof(T) * 8) - 1);

    Pothos::BufferChunk expectedLeftShiftOutput(dtype, bufferLen);
    Pothos::BufferChunk expectedRightShiftOutput(dtype, bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        expectedLeftShiftOutput.template as<T*>()[elem] = input.template as<T*>()[elem] << leftShiftSize;
        expectedRightShiftOutput.template as<T*>()[elem] = input.template as<T*>()[elem] >> rightShiftSize;
    }

    auto source = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    source.call("feedBuffer", input);

    auto leftShift = Pothos::BlockRegistry::make("/comms/bitshift", dtype, "LEFTSHIFT", leftShiftSize);
    auto leftShiftSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    POTHOS_TEST_EQUAL(leftShiftSize, leftShift.call<size_t>("shiftSize"));

    auto rightShift = Pothos::BlockRegistry::make("/comms/bitshift", dtype, "RIGHTSHIFT", rightShiftSize);
    auto rightShiftSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    POTHOS_TEST_EQUAL(rightShiftSize, rightShift.call<size_t>("shiftSize"));

    {
        Pothos::Topology topology;

        topology.connect(source, 0, leftShift, 0);
        topology.connect(leftShift, 0, leftShiftSink, 0);

        topology.connect(source, 0, rightShift, 0);
        topology.connect(rightShift, 0, rightShiftSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    std::cout << " * Testing LEFTSHIFT..." << std::endl;
    CommsTests::testBufferChunksEqual<T>(
        expectedLeftShiftOutput,
        leftShiftSink.call<Pothos::BufferChunk>("getBuffer"));

    std::cout << " * Testing RIGHTSHIFT..." << std::endl;
    CommsTests::testBufferChunksEqual<T>(
        expectedRightShiftOutput,
        rightShiftSink.call<Pothos::BufferChunk>("getBuffer"));
}

//
// Tests
//

POTHOS_TEST_BLOCK("/comms/tests", test_bitwise_unary)
{
    testBitwiseUnaryArray<std::int8_t>();
    testBitwiseUnaryArray<std::int16_t>();
    testBitwiseUnaryArray<std::int32_t>();
    testBitwiseUnaryArray<std::int64_t>();
    testBitwiseUnaryArray<std::uint8_t>();
    testBitwiseUnaryArray<std::uint16_t>();
    testBitwiseUnaryArray<std::uint32_t>();
    testBitwiseUnaryArray<std::uint64_t>();
}

POTHOS_TEST_BLOCK("/comms/tests", test_bitwise_binary)
{
    testBitwiseBinaryArray<std::int8_t>();
    testBitwiseBinaryArray<std::int16_t>();
    testBitwiseBinaryArray<std::int32_t>();
    testBitwiseBinaryArray<std::int64_t>();
    testBitwiseBinaryArray<std::uint8_t>();
    testBitwiseBinaryArray<std::uint16_t>();
    testBitwiseBinaryArray<std::uint32_t>();
    testBitwiseBinaryArray<std::uint64_t>();
}

POTHOS_TEST_BLOCK("/comms/tests", test_bitwise_const_binary)
{
    testBitwiseBinaryConst<std::int8_t>();
    testBitwiseBinaryConst<std::int16_t>();
    testBitwiseBinaryConst<std::int32_t>();
    testBitwiseBinaryConst<std::int64_t>();
    testBitwiseBinaryConst<std::uint8_t>();
    testBitwiseBinaryConst<std::uint16_t>();
    testBitwiseBinaryConst<std::uint32_t>();
    testBitwiseBinaryConst<std::uint64_t>();
}

POTHOS_TEST_BLOCK("/comms/tests", test_bitshift)
{
    testBitShift<std::int8_t>();
    testBitShift<std::int16_t>();
    testBitShift<std::int32_t>();
    testBitShift<std::int64_t>();
    testBitShift<std::uint8_t>();
    testBitShift<std::uint16_t>();
    testBitShift<std::uint32_t>();
    testBitShift<std::uint64_t>();
}
