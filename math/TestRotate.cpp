// Copyright (c) 2015-2018 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <cstdint>
#include <complex>
#include <cmath>
#include <iostream>

static const size_t NUM_POINTS = 13;

template <typename Type>
void testRotateTmpl(const double phase)
{
    auto dtype = Pothos::DType(typeid(std::complex<Type>));
    std::cout << "Testing rotate with type " << dtype.toString() << ", phase " << (phase/M_PI) << "*pi" << std::endl;

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto rotate = Pothos::BlockRegistry::make("/comms/rotate", dtype);
    rotate.call("setPhase", phase);
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    //load the feeder
    auto buffIn = Pothos::BufferChunk(typeid(std::complex<Type>), NUM_POINTS);
    auto pIn = buffIn.as<std::complex<Type> *>();
    for (size_t i = 0; i < buffIn.elements(); i++)
    {
        const auto f = Type(i); //cast prevents unsigned type promotion in multiplication
        pIn[i] = std::complex<Type>(Type(10*f), Type(-20*f));
    }
    feeder.call("feedBuffer", buffIn);

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, rotate, 0);
        topology.connect(rotate, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    //check the collector
    Pothos::BufferChunk buffOut = collector.call("getBuffer");
    POTHOS_TEST_EQUAL(buffOut.elements(), buffIn.elements());
    auto pOut = buffOut.as<const std::complex<Type> *>();
    for (size_t i = 0; i < buffOut.elements(); i++)
    {
        const auto input = std::complex<double>(pIn[i].real(), pIn[i].imag());
        const auto expected = std::complex<Type>(input * std::polar(1.0, phase));
        //allow up to an error of 1 because of fixed point truncation rounding
        POTHOS_TEST_CLOSE(pOut[i], expected, 1);
    }
}

POTHOS_TEST_BLOCK("/comms/tests", test_rotate)
{
    for (size_t i = 0; i < 4; i++)
    {
        const double phase = i*M_PI/2;
        testRotateTmpl<double>(phase);
        testRotateTmpl<float>(phase);
        testRotateTmpl<int64_t>(phase);
        testRotateTmpl<int32_t>(phase);
        testRotateTmpl<int16_t>(phase);
        testRotateTmpl<int8_t>(phase);
    }
}
