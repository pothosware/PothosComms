// Copyright (c) 2014-2016 Josh Blum
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>

#include <algorithm>
#include <iostream>

POTHOS_TEST_BLOCK("/comms/tests", test_arithmetic_add)
{
    auto feeder0 = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto feeder1 = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto adder = Pothos::BlockRegistry::make("/comms/arithmetic", "int", "ADD");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");

    //load feeder blocks
    auto b0 = Pothos::BufferChunk(10*sizeof(int));
    auto p0 = b0.as<int *>();
    for (size_t i = 0; i < 10; i++) p0[i] = i;
    feeder0.call("feedBuffer", b0);

    auto b1 = Pothos::BufferChunk(10*sizeof(int));
    auto p1 = b1.as<int *>();
    for (size_t i = 0; i < 10; i++) p1[i] = i+10;
    feeder1.call("feedBuffer", b1);

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder0, 0, adder, 0);
        topology.connect(feeder1, 0, adder, 1);
        topology.connect(adder, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    //check the collector
    Pothos::BufferChunk buff = collector.call("getBuffer");
    POTHOS_TEST_EQUAL(buff.length, 10*sizeof(int));
    auto pb = buff.as<const int *>();
    //for (int i = 0; i < 10; i++) std::cout << i << " " << pb[i] << std::endl;
    for (int i = 0; i < 10; i++) POTHOS_TEST_EQUAL(pb[i], i+i+10);
}

POTHOS_TEST_BLOCK("/comms/tests", test_arithmetic_feedback)
{
    auto registry = Pothos::ProxyEnvironment::make("managed")->findProxy("Pothos/BlockRegistry");

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto adder = Pothos::BlockRegistry::make("/comms/arithmetic", "int", "ADD");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");

    //adder has a preload on input1 for feedback loop
    std::vector<size_t> preload(2, 0); preload[1] = 1;
    adder.call("setPreload", preload);

    //load feeder block
    auto b0 = Pothos::BufferChunk(10*sizeof(int));
    auto p0 = b0.as<int *>();
    for (size_t i = 0; i < 10; i++) p0[i] = i;
    feeder.call("feedBuffer", b0);

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, adder, 0);
        topology.connect(adder, 0, adder, 1);
        topology.connect(adder, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    //check the collector
    Pothos::BufferChunk buff = collector.call("getBuffer");
    POTHOS_TEST_EQUAL(buff.length, 10*sizeof(int));
    auto pb = buff.as<const int *>();
    //for (int i = 0; i < 10; i++) std::cout << i << " " << pb[i] << std::endl;
    int last = 0;
    for (int i = 0; i < 10; i++)
    {
        last = i + last;
        POTHOS_TEST_EQUAL(pb[i], last);
    }
}

POTHOS_TEST_BLOCK("/comms/tests", test_inline_buffer)
{
    auto feeder0 = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto feeder1 = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto copier = Pothos::BlockRegistry::make("/blocks/copier");
    auto adder = Pothos::BlockRegistry::make("/comms/arithmetic", "int", "ADD");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");

    //load feeder blocks
    const auto numElems = 4000;
    auto b0 = Pothos::BufferChunk(numElems*sizeof(int));
    auto p0 = b0.as<int *>();
    for (size_t i = 0; i < numElems; i++) p0[i] = i;
    feeder0.call("feedBuffer", b0);

    auto b1 = Pothos::BufferChunk(numElems*sizeof(int));
    auto p1 = b1.as<int *>();
    for (size_t i = 0; i < numElems; i++) p1[i] = i+numElems;
    //feeder1.call("feedBuffer", b1);

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder0, 0, copier, 0); //copier before adder ensures framework provided buffers
        topology.connect(copier, 0, adder, 0);
        topology.connect(feeder1, 0, adder, 1);
        topology.connect(adder, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
        feeder1.call("feedBuffer", b1); //ensure that the buffer will be inlined by forcing processing on a non port 0 message
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    //check the collector
    Pothos::BufferChunk buff = collector.call("getBuffer");
    POTHOS_TEST_EQUAL(buff.length, numElems*sizeof(int));
    auto pb = buff.as<const int *>();
    //for (int i = 0; i < numElems; i++) std::cout << i << " " << pb[i] << std::endl;
    for (int i = 0; i < numElems; i++) POTHOS_TEST_EQUAL(pb[i], i+i+numElems);

    size_t numInlines = adder.call("getNumInlineBuffers");
    std::cout << "NumInlineBuffers " << numInlines << std::endl;
    POTHOS_TEST_TRUE(numInlines > 0);
}

static void testConstArithmetic(
    std::int32_t constant,
    const std::string& operation,
    const std::vector<std::int32_t>& inputs,
    const std::vector<std::int32_t>& expectedOutputs)
{
    std::cout << "operation = " << operation << "..." << std::endl;
    POTHOS_TEST_EQUAL(inputs.size(), expectedOutputs.size());

    static const Pothos::DType dtype("int32");

    Pothos::BufferChunk bufferChunk(dtype, inputs.size());
    for(size_t i = 0; i < inputs.size(); ++i)
    {
        bufferChunk.as<std::int32_t*>()[i] = inputs[i];
    }

    auto feederSource = Pothos::BlockRegistry::make(
                            "/blocks/feeder_source",
                            dtype);
    feederSource.call("feedBuffer", bufferChunk);

    auto collectorSink = Pothos::BlockRegistry::make(
                             "/blocks/collector_sink",
                             dtype);

    auto constArithmetic = Pothos::BlockRegistry::make(
                               "/comms/const_arithmetic",
                               dtype,
                               operation,
                               0);

    constArithmetic.call("setConstant", constant);
    POTHOS_TEST_EQUAL(constant, constArithmetic.call<std::int32_t>("constant"));

    {
        Pothos::Topology topology;

        topology.connect(
            feederSource, 0,
            constArithmetic, 0);
        topology.connect(
            constArithmetic, 0,
            collectorSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    Pothos::BufferChunk outputs = collectorSink.call("getBuffer");
    POTHOS_TEST_EQUAL(expectedOutputs.size(), outputs.elements());
    POTHOS_TEST_EQUALA(
        expectedOutputs.data(),
        outputs.as<const std::int32_t*>(),
        expectedOutputs.size());
}

POTHOS_TEST_BLOCK("/comms/tests", test_const_arithmetic)
{
    const std::int32_t constant = 5;
    const std::vector<std::int32_t> inputs = {1,2,3,4,5,6,7,8,9,10};

    std::vector<std::int32_t> XPlusKOutputs;
    std::vector<std::int32_t> XSubKOutputs;
    std::vector<std::int32_t> KSubXOutputs;
    std::vector<std::int32_t> XMultKOutputs;
    std::vector<std::int32_t> XDivKOutputs;
    std::vector<std::int32_t> KDivXOutputs;
    
    std::transform(
        inputs.begin(),
        inputs.end(),
        std::back_inserter(XPlusKOutputs),
        [&constant](std::int32_t val){return val+constant;});
    std::transform(
        inputs.begin(),
        inputs.end(),
        std::back_inserter(XSubKOutputs),
        [&constant](std::int32_t val){return val-constant;});
    std::transform(
        inputs.begin(),
        inputs.end(),
        std::back_inserter(KSubXOutputs),
        [&constant](std::int32_t val){return constant-val;});
    std::transform(
        inputs.begin(),
        inputs.end(),
        std::back_inserter(XMultKOutputs),
        [&constant](std::int32_t val){return val*constant;});
    std::transform(
        inputs.begin(),
        inputs.end(),
        std::back_inserter(XDivKOutputs),
        [&constant](std::int32_t val){return val/constant;});
    std::transform(
        inputs.begin(),
        inputs.end(),
        std::back_inserter(KDivXOutputs),
        [&constant](std::int32_t val){return constant/val;});
    
    testConstArithmetic(constant, "X+K", inputs, XPlusKOutputs);
    testConstArithmetic(constant, "X-K", inputs, XSubKOutputs);
    testConstArithmetic(constant, "K-X", inputs, KSubXOutputs);
    testConstArithmetic(constant, "X*K", inputs, XMultKOutputs);
    testConstArithmetic(constant, "X/K", inputs, XDivKOutputs);
    testConstArithmetic(constant, "K/X", inputs, KDivXOutputs);
}
