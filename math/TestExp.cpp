// Copyright (c) 2021 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "Exp10.hpp"

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>

#include <Poco/Random.h>

#include <cstdint>
#include <cmath>
#include <iostream>
#include <type_traits>

constexpr size_t bufferLen = 100;

//
// Utility code
//

// Make it easier to templatize
template <typename Type>
static double expTmpl(Type input)
{
    return std::exp(input);
}

// Make it easier to templatize
template <typename Type>
static double exp2Tmpl(Type input)
{
    return std::exp2(input);
}

// Make it easier to templatize
template <typename Type>
static double exp10Tmpl(Type input)
{
    return detail::exp10(input);
}

// Make it easier to templatize
template <typename Type>
static double expm1Tmpl(Type input)
{
    return std::expm1(input);
}

// Make it easier to templatize
template <typename Type>
static double expNTmpl(Type input, Type base)
{
    return std::pow(base, input);
}

static Poco::Random rng;

template <typename T>
constexpr typename std::enable_if<std::is_integral<T>::value, T>::type maxRandNum()
{
    return (sizeof(T) == 1) ? 2 : 5;
}

template <typename T>
constexpr typename std::enable_if<std::is_floating_point<T>::value, T>::type maxRandNum()
{
    return T(100);
}

template <typename T>
static inline typename std::enable_if<std::is_integral<T>::value, T>::type randNum()
{
    T ret = T(rng.next(maxRandNum<T>()));
    if(std::is_signed<T>::value) ret -= maxRandNum<T>();

    return ret;
}

template <typename T>
static inline typename std::enable_if<std::is_floating_point<T>::value, T>::type randNum()
{
    // nextFloat() return a value [0,1]
    return (rng.nextFloat() * maxRandNum<T>()) - maxRandNum<T>();
}

//
// Test implementations
//

template <typename Type>
using ExpTmplFcn = double(*)(Type);

template <typename Type>
static void testExpNImpl(Type base)
{
    const auto blockPath = "/comms/expN";
    const auto dtype = Pothos::DType(typeid(Type));
    std::cout << "Testing " << blockPath << " with type " << dtype.toString()
              << " and base " << Pothos::Object(base).toString() << std::endl;

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    auto exp = Pothos::BlockRegistry::make(blockPath, dtype, base);
    POTHOS_TEST_EQUAL(base, exp.template call<Type>("base"));

    Pothos::BufferChunk abuffOut = collector.call("getBuffer");
    // Load the feeder
    auto buffIn = Pothos::BufferChunk(typeid(Type), bufferLen);
    auto pIn = buffIn.as<Type *>();
    for (size_t i = 0; i < buffIn.elements(); i++)
    {
        pIn[i] = randNum<Type>();
    }
    feeder.call("feedBuffer", buffIn);

    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, exp, 0);
        topology.connect(exp   , 0, collector, 0);
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
        const auto expected = expNTmpl(pIn[i], base);
        POTHOS_TEST_CLOSE(pOut[i], expected, 1);
    }
}

template <typename Type>
static void testFixedBaseImpl(const std::string& blockPath, ExpTmplFcn<Type> expFcn)
{
    static const auto dtype = Pothos::DType(typeid(Type));
    std::cout << "Testing " << blockPath << " with type " << dtype.toString() << std::endl;

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto exp = Pothos::BlockRegistry::make(blockPath, dtype);
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    Pothos::BufferChunk abuffOut = collector.call("getBuffer");
    //load the feeder
    auto buffIn = Pothos::BufferChunk(typeid(Type), bufferLen);
    auto pIn = buffIn.as<Type *>();
    for (size_t i = 0; i < buffIn.elements(); i++)
    {
        pIn[i] = randNum<Type>();
    }
    feeder.call("feedBuffer", buffIn);

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, exp, 0);
        topology.connect(exp   , 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    //check the collector
    Pothos::BufferChunk buffOut = collector.call("getBuffer");
    POTHOS_TEST_EQUAL(buffOut.elements(), buffIn.elements());
    auto pOut = buffOut.as<const Type *>();
    for (size_t i = 0; i < buffOut.elements(); i++)
    {
        const auto expected = expFcn(pIn[i]);
        //allow up to an error of 1 because of fixed point truncation rounding
        POTHOS_TEST_CLOSE(pOut[i], expected, 1);
    }
}

POTHOS_TEST_BLOCK("/comms/tests", test_exp)
{
    testFixedBaseImpl<double>("/comms/exp", &expTmpl<double>);
    testFixedBaseImpl<float>("/comms/exp", &expTmpl<float>);
    testFixedBaseImpl<int64_t>("/comms/exp", &expTmpl<int64_t>);
    testFixedBaseImpl<int32_t>("/comms/exp", &expTmpl<int32_t>);
    testFixedBaseImpl<int16_t>("/comms/exp", &expTmpl<int16_t>);
    testFixedBaseImpl<int8_t>("/comms/exp", &expTmpl<int8_t>);
    testFixedBaseImpl<uint64_t>("/comms/exp", &expTmpl<uint64_t>);
    testFixedBaseImpl<uint32_t>("/comms/exp", &expTmpl<uint32_t>);
    testFixedBaseImpl<uint16_t>("/comms/exp", &expTmpl<uint16_t>);
    testFixedBaseImpl<uint8_t>("/comms/exp", &expTmpl<uint8_t>);
}

POTHOS_TEST_BLOCK("/comms/tests", test_exp2)
{
    testFixedBaseImpl<double>("/comms/exp2", &exp2Tmpl<double>);
    testFixedBaseImpl<float>("/comms/exp2", &exp2Tmpl<float>);
    testFixedBaseImpl<int64_t>("/comms/exp2", &exp2Tmpl<int64_t>);
    testFixedBaseImpl<int32_t>("/comms/exp2", &exp2Tmpl<int32_t>);
    testFixedBaseImpl<int16_t>("/comms/exp2", &exp2Tmpl<int16_t>);
    testFixedBaseImpl<int8_t>("/comms/exp2", &exp2Tmpl<int8_t>);
    testFixedBaseImpl<uint64_t>("/comms/exp2", &exp2Tmpl<uint64_t>);
    testFixedBaseImpl<uint32_t>("/comms/exp2", &exp2Tmpl<uint32_t>);
    testFixedBaseImpl<uint16_t>("/comms/exp2", &exp2Tmpl<uint16_t>);
    testFixedBaseImpl<uint8_t>("/comms/exp2", &exp2Tmpl<uint8_t>);
}

POTHOS_TEST_BLOCK("/comms/tests", test_exp10)
{
    testFixedBaseImpl<double>("/comms/exp10", &exp10Tmpl<double>);
    testFixedBaseImpl<float>("/comms/exp10", &exp10Tmpl<float>);
    testFixedBaseImpl<int64_t>("/comms/exp10", &exp10Tmpl<int64_t>);
    testFixedBaseImpl<int32_t>("/comms/exp10", &exp10Tmpl<int32_t>);
    testFixedBaseImpl<int16_t>("/comms/exp10", &exp10Tmpl<int16_t>);
    testFixedBaseImpl<int8_t>("/comms/exp10", &exp10Tmpl<int8_t>);
    testFixedBaseImpl<uint64_t>("/comms/exp10", &exp10Tmpl<uint64_t>);
    testFixedBaseImpl<uint32_t>("/comms/exp10", &exp10Tmpl<uint32_t>);
    testFixedBaseImpl<uint16_t>("/comms/exp10", &exp10Tmpl<uint16_t>);
    testFixedBaseImpl<uint8_t>("/comms/exp10", &exp10Tmpl<uint8_t>);
}

POTHOS_TEST_BLOCK("/comms/tests", test_expm1)
{
    testFixedBaseImpl<double>("/comms/expm1", &expm1Tmpl<double>);
    testFixedBaseImpl<float>("/comms/expm1", &expm1Tmpl<float>);
    testFixedBaseImpl<int64_t>("/comms/expm1", &expm1Tmpl<int64_t>);
    testFixedBaseImpl<int32_t>("/comms/expm1", &expm1Tmpl<int32_t>);
    testFixedBaseImpl<int16_t>("/comms/expm1", &expm1Tmpl<int16_t>);
    testFixedBaseImpl<int8_t>("/comms/expm1", &expm1Tmpl<int8_t>);
    testFixedBaseImpl<uint64_t>("/comms/expm1", &expm1Tmpl<uint64_t>);
    testFixedBaseImpl<uint32_t>("/comms/expm1", &expm1Tmpl<uint32_t>);
    testFixedBaseImpl<uint16_t>("/comms/expm1", &expm1Tmpl<uint16_t>);
    testFixedBaseImpl<uint8_t>("/comms/expm1", &expm1Tmpl<uint8_t>);
}

POTHOS_TEST_BLOCK("/comms/tests", test_expN)
{
    for(size_t base = 2; base <= 10; ++base)
    {
        testExpNImpl<double>(double(base));
        testExpNImpl<float>(float(base));
        testExpNImpl<int64_t>(int64_t(base));
        testExpNImpl<int32_t>(int32_t(base));
        testExpNImpl<int16_t>(int16_t(base));
        testExpNImpl<int8_t>(int8_t(base));
        testExpNImpl<uint64_t>(uint64_t(base));
        testExpNImpl<uint32_t>(uint32_t(base));
        testExpNImpl<uint16_t>(uint16_t(base));
        testExpNImpl<uint8_t>(uint8_t(base));
    }
}
