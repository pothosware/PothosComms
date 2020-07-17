// Copyright (c) 2019 Nick Foster
//               2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <cstdint>
#include <cmath>
#include <iostream>

static constexpr size_t NUM_POINTS = 12;

//
// Utility code
//

// Make it easier to templatize
template <typename Type>
static double logTmpl(Type input)
{
    return std::log(input);
}


// Make it easier to templatize
template <typename Type>
static double log2Tmpl(Type input)
{
    return std::log2(input);
}

// Make it easier to templatize
template <typename Type>
static double log10Tmpl(Type input)
{
    return std::log10(input);
}

// Make it easier to templatize
template <typename Type>
static double logNTmpl(Type input, Type base)
{
    return std::log(input) / std::log(base);
}


//
// Test implementations
//

template <typename Type>
using LogTmplFcn = double(*)(Type);

template <typename Type>
static void testLogNImpl(Type base)
{
    const auto blockPath = "/comms/logN";
    const auto dtype = Pothos::DType(typeid(Type));
    std::cout << "Testing " << blockPath << " with type " << dtype.toString()
              << " and base " << Pothos::Object(base).toString() << std::endl;

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    auto log = Pothos::BlockRegistry::make(blockPath, dtype, base);
    POTHOS_TEST_EQUAL(base, log.template call<Type>("base"));

    Pothos::BufferChunk abuffOut = collector.call("getBuffer");
    // Load the feeder
    auto buffIn = Pothos::BufferChunk(typeid(Type), NUM_POINTS);
    auto pIn = buffIn.as<Type *>();
    for (size_t i = 0; i < buffIn.elements(); i++)
    {
        pIn[i] = Type(10*(i+1));
    }
    feeder.call("feedBuffer", buffIn);

    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, log, 0);
        topology.connect(log   , 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    // Check the collector
    Pothos::BufferChunk buffOut = collector.call("getBuffer");
    POTHOS_TEST_EQUAL(buffOut.elements(), buffIn.elements());
    auto pOut = buffOut.as<const Type *>();
    for (size_t i = 0; i < buffOut.elements(); i++)
    {
        // Allow up to an error of 1 because of fixed point truncation rounding
        const auto expected = logNTmpl(pIn[i], base);
        POTHOS_TEST_CLOSE(pOut[i], expected, 1);
    }
}

template <typename Type>
static void testFixedBaseImpl(const std::string& blockPath, LogTmplFcn<Type> logFcn)
{
    static const auto dtype = Pothos::DType(typeid(Type));
    std::cout << "Testing " << blockPath << " with type " << dtype.toString() << std::endl;

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto log = Pothos::BlockRegistry::make(blockPath, dtype);
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    Pothos::BufferChunk abuffOut = collector.call("getBuffer");
    //load the feeder
    auto buffIn = Pothos::BufferChunk(typeid(Type), NUM_POINTS);
    auto pIn = buffIn.as<Type *>();
    for (size_t i = 0; i < buffIn.elements(); i++)
    {
        pIn[i] = Type(10*(i+1));
    }
    feeder.call("feedBuffer", buffIn);

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, log, 0);
        topology.connect(log   , 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    //check the collector
    Pothos::BufferChunk buffOut = collector.call("getBuffer");
    POTHOS_TEST_EQUAL(buffOut.elements(), buffIn.elements());
    auto pOut = buffOut.as<const Type *>();
    for (size_t i = 0; i < buffOut.elements(); i++)
    {
        const auto expected = logFcn(pIn[i]);
        //allow up to an error of 1 because of fixed point truncation rounding
        POTHOS_TEST_CLOSE(pOut[i], expected, 1);
    }
}

POTHOS_TEST_BLOCK("/comms/tests", test_log)
{
    testFixedBaseImpl<double>("/comms/log", &logTmpl<double>);
    testFixedBaseImpl<float>("/comms/log", &logTmpl<float>);
    testFixedBaseImpl<int64_t>("/comms/log", &logTmpl<int64_t>);
    testFixedBaseImpl<int32_t>("/comms/log", &logTmpl<int32_t>);
    testFixedBaseImpl<int16_t>("/comms/log", &logTmpl<int16_t>);
    testFixedBaseImpl<int8_t>("/comms/log", &logTmpl<int8_t>);
    testFixedBaseImpl<uint64_t>("/comms/log", &logTmpl<uint64_t>);
    testFixedBaseImpl<uint32_t>("/comms/log", &logTmpl<uint32_t>);
    testFixedBaseImpl<uint16_t>("/comms/log", &logTmpl<uint16_t>);
    testFixedBaseImpl<uint8_t>("/comms/log", &logTmpl<uint8_t>);
}

POTHOS_TEST_BLOCK("/comms/tests", test_log2)
{
    testFixedBaseImpl<double>("/comms/log2", &log2Tmpl<double>);
    testFixedBaseImpl<float>("/comms/log2", &log2Tmpl<float>);
    testFixedBaseImpl<int64_t>("/comms/log2", &log2Tmpl<int64_t>);
    testFixedBaseImpl<int32_t>("/comms/log2", &log2Tmpl<int32_t>);
    testFixedBaseImpl<int16_t>("/comms/log2", &log2Tmpl<int16_t>);
    testFixedBaseImpl<int8_t>("/comms/log2", &log2Tmpl<int8_t>);
    testFixedBaseImpl<uint64_t>("/comms/log2", &log2Tmpl<uint64_t>);
    testFixedBaseImpl<uint32_t>("/comms/log2", &log2Tmpl<uint32_t>);
    testFixedBaseImpl<uint16_t>("/comms/log2", &log2Tmpl<uint16_t>);
    testFixedBaseImpl<uint8_t>("/comms/log2", &log2Tmpl<uint8_t>);
}

POTHOS_TEST_BLOCK("/comms/tests", test_log10)
{
    testFixedBaseImpl<double>("/comms/log10", &log10Tmpl<double>);
    testFixedBaseImpl<float>("/comms/log10", &log10Tmpl<float>);
    testFixedBaseImpl<int64_t>("/comms/log10", &log10Tmpl<int64_t>);
    testFixedBaseImpl<int32_t>("/comms/log10", &log10Tmpl<int32_t>);
    testFixedBaseImpl<int16_t>("/comms/log10", &log10Tmpl<int16_t>);
    testFixedBaseImpl<int8_t>("/comms/log10", &log10Tmpl<int8_t>);
    testFixedBaseImpl<uint64_t>("/comms/log10", &log10Tmpl<uint64_t>);
    testFixedBaseImpl<uint32_t>("/comms/log10", &log10Tmpl<uint32_t>);
    testFixedBaseImpl<uint16_t>("/comms/log10", &log10Tmpl<uint16_t>);
    testFixedBaseImpl<uint8_t>("/comms/log10", &log10Tmpl<uint8_t>);
}

POTHOS_TEST_BLOCK("/comms/tests", test_logN)
{
    for(size_t base = 2; base <= 10; ++base)
    {
        testLogNImpl<double>(double(base));
        testLogNImpl<float>(float(base));
        testLogNImpl<int64_t>(int64_t(base));
        testLogNImpl<int32_t>(int32_t(base));
        testLogNImpl<int16_t>(int16_t(base));
        testLogNImpl<int8_t>(int8_t(base));
        testLogNImpl<uint64_t>(uint64_t(base));
        testLogNImpl<uint32_t>(uint32_t(base));
        testLogNImpl<uint16_t>(uint16_t(base));
        testLogNImpl<uint8_t>(uint8_t(base));
    }
}
