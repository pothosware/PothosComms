// Copyright (c) 2015-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <iostream>
#include <complex>

POTHOS_TEST_BLOCK("/comms/tests", test_preamble_correlator)
{
    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "unsigned char");
    auto correlator = Pothos::BlockRegistry::make("/comms/preamble_correlator");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "unsigned char");

    const std::vector<unsigned char> preamble{0, 1, 1, 1, 1, 0};
    size_t testLength = 10 + preamble.size();
    size_t preambleIndex = 4;

    correlator.call("setPreamble", preamble);
    correlator.call("setThreshold", 0);

    //load feeder blocks
    auto b0 = Pothos::BufferChunk(testLength + preamble.size());
    auto p0 = b0.as<unsigned char *>();
    for (size_t i = 0; i < testLength; i++) p0[i] = i % 2;
    for (size_t i = 0; i < preamble.size(); i++) p0[i + preambleIndex] = preamble[i];
    feeder.call("feedBuffer", b0);

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, correlator, 0);
        topology.connect(correlator, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    //check the collector buffer matches input
    //the last preamble-sized window of elements is left in the correlator
    Pothos::BufferChunk buff = collector.call("getBuffer");
    POTHOS_TEST_EQUAL(testLength, buff.elements());
    auto pb = buff.as<const unsigned char *>();
    POTHOS_TEST_EQUALA(pb, p0, testLength);

    //check for the preamble label
    std::vector<Pothos::Label> labels = collector.call("getLabels");
    POTHOS_TEST_EQUAL(labels.size(), 1);
    POTHOS_TEST_EQUAL(labels[0].index, preambleIndex + preamble.size());
}
