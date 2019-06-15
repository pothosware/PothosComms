// Copyright (c) 2019 Nick Foster
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <cstdint>
#include <cmath>
#include <iostream>

static const size_t NUM_POINTS = 12;

template <typename Type>
void testLog10Tmpl(void)
{
    auto dtype = Pothos::DType(typeid(Type));
    std::cout << "Testing log10 with type " << dtype.toString() << std::endl;

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto log10 = Pothos::BlockRegistry::make("/comms/log10", dtype);
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
        topology.connect(feeder, 0, log10, 0);
        topology.connect(log10 , 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    //check the collector
    Pothos::BufferChunk buffOut = collector.call("getBuffer");
    POTHOS_TEST_EQUAL(buffOut.elements(), buffIn.elements());
    auto pOut = buffOut.as<const Type *>();
    for (size_t i = 0; i < buffOut.elements(); i++)
    {
        const auto expected = std::log10(pIn[i]);
        //allow up to an error of 1 because of fixed point truncation rounding
        POTHOS_TEST_CLOSE(pOut[i], expected, 1);
    }
}

POTHOS_TEST_BLOCK("/comms/tests", test_log10)
{
    for (size_t i = 0; i <= 4; i++)
    {
        testLog10Tmpl<double>();
        testLog10Tmpl<float>();
        testLog10Tmpl<int64_t>();
        testLog10Tmpl<int32_t>();
        testLog10Tmpl<int16_t>();
        testLog10Tmpl<int8_t>();
    }
}
