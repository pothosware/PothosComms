// Copyright (c) 2021 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "common/Testing.hpp"

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>

#include <iostream>
#include <vector>

static constexpr size_t numRepetitions = 100;

template <typename Type>
static void testSigmoidImpl()
{
    // linspace(0,20,42)
    const auto inputs = CommsTests::stdVectorToStretchedBufferChunk<Type>(
        std::vector<Type>
        {
            0.0,         0.48780488,  0.97560976,  1.46341463,  1.95121951,  2.43902439,  2.92682927,
            3.41463415,  3.90243902,  4.3902439,   4.87804878,  5.36585366,  5.85365854,  6.34146341,
            6.82926829,  7.31707317,  7.80487805,  8.29268293,  8.7804878,   9.26829268,  9.75609756,
            10.24390244, 10.73170732, 11.2195122,  11.70731707, 12.19512195, 12.68292683, 13.17073171,
            13.65853659, 14.14634146, 14.63414634, 15.12195122, 15.6097561,  16.09756098, 16.58536585,
            17.07317073, 17.56097561, 18.04878049, 18.53658537, 19.02439024, 19.51219512, 20.0
        },
        numRepetitions);

    // Source: Wolfram Alpha
    const auto expectedOutputs = CommsTests::stdVectorToStretchedBufferChunk<Type>(
        std::vector<Type>
        {
            0.5,      0.619589, 0.726236, 0.812054, 0.87558,  0.919755, 0.949157,
            0.968159, 0.980207, 0.987754, 0.992446, 0.995348, 0.997139, 0.998241,
            0.99892,  0.999336, 0.999592, 0.99975,  0.999846, 0.999906, 0.999942,
            0.999964, 0.999978, 0.999987, 0.999992, 0.999995, 0.999997, 0.999998,
            0.999998, 0.999999, 1.0,      1.0,      1.0,      1.0,      1.0,
            1.0,      1.0,      1.0,      1.0,      1.0,      1.0,      1.0
        },
        numRepetitions);

    const auto dtype = Pothos::DType(typeid(Type));
    std::cout << "Testing " << dtype.toString() << "..." << std::endl;

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto sigmoid = Pothos::BlockRegistry::make("/comms/sigmoid", dtype);
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    feeder.call("feedBuffer", inputs);

    {
        Pothos::Topology topology;

        topology.connect(
            feeder, 0,
            sigmoid, 0);
        topology.connect(
            sigmoid, 0,
            collector, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    constexpr Type epsilon = 1e-3;
    CommsTests::testBufferChunksClose<Type>(
        expectedOutputs,
        collector.call<Pothos::BufferChunk>("getBuffer"),
        epsilon);
}

POTHOS_TEST_BLOCK("/comms/tests", test_sigmoid)
{
    testSigmoidImpl<float>();
    testSigmoidImpl<double>();
}
