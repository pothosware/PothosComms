// Copyright (c) 2015-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <cstdint>
#include <complex>
#include <cmath>
#include <iostream>

static const size_t NUM_POINTS = 13;
static const double ALLOWED_ERROR = M_PI/500;
static const double FXPT_SCALE = (1 << 15)/M_PI;
static const double FXPT_ERROR = ALLOWED_ERROR*FXPT_SCALE;

template <typename Type>
void testRotateTmpl(void)
{
    auto dtype = Pothos::DType(typeid(std::complex<Type>));
    std::cout << "Testing angle with type " << dtype.toString() << std::endl;

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto angle = Pothos::BlockRegistry::make("/comms/angle", dtype);
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", Pothos::DType(typeid(Type)));

    //load the feeder
    auto buffIn = Pothos::BufferChunk(typeid(std::complex<Type>), NUM_POINTS);
    auto pIn = buffIn.as<std::complex<Type> *>();
    for (size_t i = 0; i < buffIn.elements(); i++)
    {
        double mag = i*1000;
        double angle = i*(M_PI/5);
        pIn[i] = mag*std::polar(1.0, angle);
    }
    feeder.callProxy("feedBuffer", buffIn);

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, angle, 0);
        topology.connect(angle, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    //check the collector
    auto buffOut = collector.call<Pothos::BufferChunk>("getBuffer");
    POTHOS_TEST_EQUAL(buffOut.elements(), buffIn.elements());
    auto pOut = buffOut.as<const Type *>();
    for (size_t i = 0; i < buffOut.elements(); i++)
    {
        const auto input = std::complex<double>(pIn[i].real(), pIn[i].imag());
        auto expected = std::arg(input);
        if (dtype.isFloat())
        {
            POTHOS_TEST_CLOSE(pOut[i], expected, ALLOWED_ERROR);
        }
        if (dtype.isInteger())
        {
            expected *= FXPT_SCALE;
            const auto angleError = int16_t(pOut[i] - expected);
            //std::cout << "expected " << expected << ", pOut[i]" << pOut[i] << std::endl;
            POTHOS_TEST_CLOSE(angleError, 0, FXPT_ERROR);
        }
    }
}

POTHOS_TEST_BLOCK("/comms/tests", test_angle)
{
    testRotateTmpl<double>();
    testRotateTmpl<float>();
    testRotateTmpl<int64_t>();
    testRotateTmpl<int32_t>();
    testRotateTmpl<int16_t>();
}
