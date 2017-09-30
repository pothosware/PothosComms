// Copyright (c) 2015-2017 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Remote.hpp>
#include <iostream>
#include <json.hpp>

using json = nlohmann::json;

POTHOS_TEST_BLOCK("/comms/tests", test_framer_to_correlator)
{
    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "uint8");
    auto generator = Pothos::BlockRegistry::make("/blocks/packet_to_stream");
    auto framer = Pothos::BlockRegistry::make("/comms/preamble_framer");
    auto correlator = Pothos::BlockRegistry::make("/comms/preamble_correlator");
    auto deframer = Pothos::BlockRegistry::make("/blocks/stream_to_packet");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "uint8");

    //Copy block provides the loopback path:
    //Copy can cause buffer boundaries to change,
    //which helps to aid in robust testing.
    auto copier = Pothos::BlockRegistry::make("/blocks/copier");

    //configuration constants
    const size_t mtu = 107;
    const std::string txFrameStartId = "txFrameStart";
    const std::string txFrameEndId = "txFrameEnd";
    const std::string rxFrameStartId = "rxFrameStart";
    const size_t maxValue = 1;
    std::vector<unsigned char> preamble;
    for (size_t i = 0; i < 32; i++) preamble.push_back(std::rand() % (maxValue+1));

    //configure
    generator.call("setFrameStartId", txFrameStartId);
    generator.call("setFrameEndId", txFrameEndId);
    generator.call("setName", "frameGenerator");
    framer.call("setPreamble", preamble);
    framer.call("setFrameStartId", txFrameStartId);
    framer.call("setFrameEndId", txFrameEndId);
    framer.call("setPaddingSize", 10);
    correlator.call("setPreamble", preamble);
    correlator.call("setThreshold", 0); //expect perfect match
    correlator.call("setFrameStartId", rxFrameStartId);
    deframer.call("setFrameStartId", rxFrameStartId);
    deframer.call("setMTU", mtu);

    //create a test plan
    json testPlan;
    testPlan["enablePackets"] = true;
    testPlan["minValue"] = 0;
    testPlan["maxValue"] = maxValue;
    testPlan["minBufferSize"] = mtu;
    testPlan["maxBufferSize"] = mtu;
    auto expected = feeder.call("feedTestPlan", testPlan.dump());

    //because of correlation window, pad feeder to flush through last message
    Pothos::Packet paddingPacket;
    paddingPacket.payload = Pothos::BufferChunk("uint8", preamble.size());
    feeder.call("feedPacket", paddingPacket);

    //create tester topology
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, generator, 0);
        topology.connect(generator, 0, framer, 0);
        topology.connect(framer, 0, copier, 0);
        topology.connect(copier, 0, correlator, 0);
        topology.connect(correlator, 0, deframer, 0);
        topology.connect(deframer, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
        //std::cout << topology.queryJSONStats() << std::endl;
    }

    std::cout << "verifyTestPlan" << std::endl;
    collector.call("verifyTestPlan", expected);
}
