// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <Poco/Platform.h>

#include <complex>
#include <cstring>
#include <iostream>

//
// Endian-independent utility functions
//

template <typename T1, typename T2>
static std::vector<T2> reinterpretCastVector(const std::vector<T1>& input)
{
    static_assert(sizeof(T1) == sizeof(T2));
    
    return std::vector<T2>(
               reinterpret_cast<const T2*>(input.data()),
               reinterpret_cast<const T2*>(input.data()) + input.size());
}

template <typename T>
static void _getTestParameters(std::vector<T>*, std::vector<T>*)
{
}

template <>
void _getTestParameters(
    std::vector<std::uint16_t>* pInputs,
    std::vector<std::uint16_t>* pSwapped)
{
    (*pInputs)  = {0x0123, 0x4567, 0x89AB, 0xCDEF};
    (*pSwapped) = {0x2301, 0x6745, 0xAB89, 0xEFCD};
}

template <>
void _getTestParameters(
    std::vector<std::uint32_t>* pInputs,
    std::vector<std::uint32_t>* pSwapped)
{
    (*pInputs)  = {0x01234567, 0x89ABCDEF, 0x76543210, 0xFEDCBA98};
    (*pSwapped) = {0x67452301, 0xEFCDAB89, 0x10325476, 0x98BADCFE};
}

template <>
void _getTestParameters(
    std::vector<std::uint64_t>* pInputs,
    std::vector<std::uint64_t>* pSwapped)
{
    (*pInputs)  = {0x0123456789ABCDEF, 0x76543210FEDCBA98, 0xDEADBEEFDEADBEEF, 0x0F1E2D3C4B5A6978};
    (*pSwapped) = {0xEFCDAB8967452301, 0x98BADCFE10325476, 0xEFBEADDEEFBEADDE, 0x78695A4B3C2D1E0F};
}

template <>
void _getTestParameters(
    std::vector<float>* pInputs,
    std::vector<float>* pSwapped)
{
    std::vector<std::uint32_t> uintInputs;
    std::vector<std::uint32_t> uintSwapped;
    _getTestParameters(&uintInputs, &uintSwapped);
    
    (*pInputs)  = reinterpretCastVector<std::uint32_t, float>(uintInputs);
    (*pSwapped) = reinterpretCastVector<std::uint32_t, float>(uintSwapped);
}

template <>
void _getTestParameters(
    std::vector<double>* pInputs,
    std::vector<double>* pSwapped)
{
    std::vector<std::uint64_t> uintInputs;
    std::vector<std::uint64_t> uintSwapped;
    _getTestParameters(&uintInputs, &uintSwapped);
    
    (*pInputs)  = reinterpretCastVector<std::uint64_t, double>(uintInputs);
    (*pSwapped) = reinterpretCastVector<std::uint64_t, double>(uintSwapped);
}

template <typename T>
void _getTestParameters(
    std::vector<std::complex<T>>* pInputs,
    std::vector<std::complex<T>>* pSwapped)
{
    std::vector<T> scalarInputs;
    std::vector<T> scalarSwapped;
    _getTestParameters(&scalarInputs, &scalarSwapped);
    POTHOS_TEST_EQUAL(scalarInputs.size(), scalarSwapped.size());
    POTHOS_TEST_EQUAL(0, (scalarInputs.size() % 2));

    pInputs->resize(scalarInputs.size()/2);
    pSwapped->resize(scalarInputs.size()/2);

    std::memcpy(
        pInputs->data(),
        scalarInputs.data(),
        sizeof(T) * scalarInputs.size());
    std::memcpy(
        pSwapped->data(),
        scalarSwapped.data(),
        sizeof(T) * scalarSwapped.size());
}

template <>
void _getTestParameters(
    std::vector<std::complex<float>>* pInputs,
    std::vector<std::complex<float>>* pSwapped)
{
    std::vector<std::complex<std::uint32_t>> uintInputs;
    std::vector<std::complex<std::uint32_t>> uintSwapped;
    _getTestParameters(&uintInputs, &uintSwapped);

    (*pInputs)  = reinterpretCastVector<std::complex<std::uint32_t>, std::complex<float>>(uintInputs);
    (*pSwapped) = reinterpretCastVector<std::complex<std::uint32_t>, std::complex<float>>(uintSwapped);
}

template <>
void _getTestParameters(
    std::vector<std::complex<double>>* pInputs,
    std::vector<std::complex<double>>* pSwapped)
{
    std::vector<std::complex<std::uint64_t>> uintInputs;
    std::vector<std::complex<std::uint64_t>> uintSwapped;
    _getTestParameters(&uintInputs, &uintSwapped);

    (*pInputs)  = reinterpretCastVector<std::complex<std::uint64_t>, std::complex<double>>(uintInputs);
    (*pSwapped) = reinterpretCastVector<std::complex<std::uint64_t>, std::complex<double>>(uintSwapped);
}

template <typename T>
static void getTestParameters(
    Pothos::BufferChunk* pInputs,
    Pothos::BufferChunk* pSwapped)
{
    static const Pothos::DType dtype(typeid(T));

    std::vector<T> inputsVec;
    std::vector<T> swappedVec;
    _getTestParameters(&inputsVec, &swappedVec);

    (*pInputs) = Pothos::BufferChunk(dtype, inputsVec.size());
    std::memcpy(
        reinterpret_cast<void*>(pInputs->address),
        inputsVec.data(),
        pInputs->length);

    (*pSwapped) = Pothos::BufferChunk(dtype, swappedVec.size());
    std::memcpy(
        reinterpret_cast<void*>(pSwapped->address),
        swappedVec.data(),
        pSwapped->length);
}

static void setAndCheckByteOrder(
    const Pothos::Proxy& byteOrder,
    const std::string& orderName)
{
    std::cout << " * " << orderName << std::endl;

    byteOrder.call("setByteOrder", orderName);
    POTHOS_TEST_EQUAL(
        orderName,
        byteOrder.call<std::string>("getByteOrder"));
}

template <typename T>
static void blockTest(
    const Pothos::Proxy& byteOrder,
    bool expectSwap)
{
    static const Pothos::DType dtype(typeid(T));

    Pothos::BufferChunk inputs;
    Pothos::BufferChunk swapped;
    getTestParameters<T>(&inputs, &swapped);

    // Use the swapped values as the payload for a packet.
    Pothos::Packet packet;
    packet.payload = swapped;

    auto feederSource = Pothos::BlockRegistry::make(
                            "/blocks/feeder_source",
                            dtype);
    feederSource.call("feedBuffer", inputs);
    feederSource.call("feedPacket", packet);

    auto collectorSink = Pothos::BlockRegistry::make(
                             "/blocks/collector_sink",
                             dtype);

    // Execute the topology.
    {
        Pothos::Topology topology;
        topology.connect(feederSource, 0, byteOrder, 0);
        topology.connect(byteOrder, 0, collectorSink, 0);
        topology.commit();

        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    Pothos::BufferChunk outputBuffer = collectorSink.call("getBuffer");
    const Pothos::BufferChunk& expectedBufferOutput = expectSwap ? swapped : inputs;
    const Pothos::BufferChunk& expectedPacketOutput = expectSwap ? inputs : swapped;

    POTHOS_TEST_EQUAL(outputBuffer.elements(), expectedBufferOutput.elements());
    POTHOS_TEST_EQUALA(
        expectedBufferOutput.as<const T*>(),
        outputBuffer.as<const T*>(),
        outputBuffer.elements());

    const auto outputMessages = collectorSink.call<std::vector<Pothos::Packet>>("getPackets");
    POTHOS_TEST_EQUAL(1, outputMessages.size());
    POTHOS_TEST_EQUAL(outputMessages[0].payload.elements(), expectedPacketOutput.elements());
    POTHOS_TEST_EQUALA(
        expectedPacketOutput.as<const T*>(),
        outputMessages[0].payload.as<const T*>(),
        outputMessages[0].payload.elements());
}

//
// Endian-dependent utility functions
//

template <typename T>
static void testSwapOrder(const Pothos::Proxy& byteOrder)
{
    setAndCheckByteOrder(
        byteOrder,
        "Swap Order");

   blockTest<T>(
       byteOrder,
       true /*expectSwap*/); 
}

template <typename T>
static void testBigEndian(const Pothos::Proxy& byteOrder)
{
    setAndCheckByteOrder(
        byteOrder,
        "Big Endian");

    blockTest<T>(
       byteOrder,
#if defined(POCO_ARCH_BIGENDIAN)
       false /*expectSwap*/
#else
       true /*expectSwap*/
#endif
       ); 
}

template <typename T>
static void testLittleEndian(const Pothos::Proxy& byteOrder)
{
    setAndCheckByteOrder(
        byteOrder,
        "Little Endian");

    blockTest<T>(
       byteOrder,
#if defined(POCO_ARCH_BIGENDIAN)
       true /*expectSwap*/
#else
       false /*expectSwap*/
#endif
       ); 
}

template <typename T>
static void testHostOrder(const Pothos::Proxy& byteOrder)
{
    setAndCheckByteOrder(
        byteOrder,
        "Network to Host");

    blockTest<T>(
       byteOrder,
#if defined(POCO_ARCH_BIGENDIAN)
       false /*expectSwap*/
#else
       true /*expectSwap*/
#endif
       ); 
}

template <typename T>
static void testNetworkOrder(const Pothos::Proxy& byteOrder)
{
    setAndCheckByteOrder(
        byteOrder,
        "Host to Network");

    blockTest<T>(
       byteOrder,
#if defined(POCO_ARCH_BIGENDIAN)
       false /*expectSwap*/
#else
       true /*expectSwap*/
#endif
       ); 
}

//
// Top-level test functions
//

template <typename T>
static void testByteOrder()
{
    const Pothos::DType dtype(typeid(T));
    std::cout << "Testing " << dtype.name() << "..." << std::endl;

    auto byteOrder = Pothos::BlockRegistry::make(
                         "/comms/byte_order",
                         dtype);

    // Test default value.
    POTHOS_TEST_EQUAL(
        "Swap Order",
        byteOrder.call<std::string>("getByteOrder"));

    testSwapOrder<T>(byteOrder);
    testBigEndian<T>(byteOrder);
    testLittleEndian<T>(byteOrder);
    testHostOrder<T>(byteOrder);
    testNetworkOrder<T>(byteOrder);
}

POTHOS_TEST_BLOCK("/comms/tests", test_byte_order)
{
    testByteOrder<std::uint16_t>();
    testByteOrder<std::uint32_t>();
    testByteOrder<std::uint64_t>();
    testByteOrder<float>();
    testByteOrder<double>();
    testByteOrder<std::complex<std::uint16_t>>();
    testByteOrder<std::complex<std::uint32_t>>();
    testByteOrder<std::complex<std::uint64_t>>();
    testByteOrder<std::complex<float>>();
    testByteOrder<std::complex<double>>();
}
