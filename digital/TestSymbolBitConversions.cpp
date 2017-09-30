// Copyright (c) 2015-2015 Rinat Zakirov
// Copyright (c) 2015-2017 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Remote.hpp>
#include <iostream>
#include <json.hpp>

using json = nlohmann::json;

POTHOS_TEST_BLOCK("/comms/tests", test_symbol_bit_conversions)
{
    //run the topology
    for (int mod = 1; mod <= 8; mod++)
    for (int i = 0; i < 2; i++)
    {
        const std::string order = i == 0 ? "LSBit" : "MSBit";
        std::cout << "run the topology with " << order << " order ";
        std::cout << "and " << mod << " modulus" << std::endl;

        //create the blocks
        auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "uint8");
        auto symsToBits = Pothos::BlockRegistry::make("/comms/symbols_to_bits");
        symsToBits.call("setModulus", mod);
        symsToBits.call("setBitOrder", order);
        auto bitsToSyms = Pothos::BlockRegistry::make("/comms/bits_to_symbols");
        bitsToSyms.call("setModulus", mod);
        bitsToSyms.call("setBitOrder", order);
        auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "uint8");

        //setup the topology
        Pothos::Topology topology;
        topology.connect(feeder, 0, symsToBits, 0);
        topology.connect(symsToBits, 0, bitsToSyms, 0);
        topology.connect(bitsToSyms, 0, collector, 0);

        //create a test plan for streams
        std::cout << "Perform stream-based test plan..." << std::endl;
        json testPlan0;
        testPlan0["enableBuffers"] = true;
        testPlan0["enableLabels"] = true;
        testPlan0["minValue"] = 0;
        testPlan0["maxValue"] = (1 << mod) - 1;
        auto expected0 = feeder.call("feedTestPlan", testPlan0.dump());
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
        collector.call("verifyTestPlan", expected0);

        //create a test plan for packets
        std::cout << "Perform packet-based test plan..." << std::endl;
        json testPlan1;
        testPlan1["enablePackets"] = true;
        testPlan1["enableLabels"] = true;
        testPlan1["minValue"] = 0;
        testPlan1["maxValue"] = (1 << mod) - 1;
        auto expected1 = feeder.call("feedTestPlan", testPlan1.dump());
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
        collector.call("verifyTestPlan", expected1);
    }

    std::cout << "done!\n";
}
