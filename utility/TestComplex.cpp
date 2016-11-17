// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Remote.hpp>
#include <Poco/JSON/Object.h>
#include <iostream>

POTHOS_TEST_BLOCK("/comms/tests", test_complex_split_combine)
{
    auto feederRe = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto collectorRe = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");
    auto feederIm = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto collectorIm = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");
    auto combineComplex = Pothos::BlockRegistry::make("/comms/combine_complex", "int");
    auto splitComplex = Pothos::BlockRegistry::make("/comms/split_complex", "int");

    //create a test plan
    //fix the size of the buffers since we use the plan on both channels
    Poco::JSON::Object::Ptr testPlan(new Poco::JSON::Object());
    testPlan->set("enableBuffers", true);
    testPlan->set("minBuffers", 3);
    testPlan->set("maxBuffers", 3);
    testPlan->set("minBufferSize", 100);
    testPlan->set("maxBufferSize", 100);

    //run the topology
    std::cout << "run the topology\n";
    {
        Pothos::Topology topology;
        topology.connect(feederRe, 0, combineComplex, "re");
        topology.connect(feederIm, 0, combineComplex, "im");
        topology.connect(combineComplex, 0, splitComplex, 0);
        topology.connect(splitComplex, "re", collectorRe, 0);
        topology.connect(splitComplex, "im", collectorIm, 0);
        topology.commit();

        auto expectedRe = feederRe.callProxy("feedTestPlan", testPlan);
        auto expectedIm = feederIm.callProxy("feedTestPlan", testPlan);
        POTHOS_TEST_TRUE(topology.waitInactive());

        std::cout << "verifyTestPlan!\n";
        collectorRe.callVoid("verifyTestPlan", expectedRe);
        collectorIm.callVoid("verifyTestPlan", expectedIm);
    }

    std::cout << "done!\n";
}
