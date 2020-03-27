// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>

#include <iostream>
#include <vector>

template <typename Type>
void testSincTmpl()
{
    /*
     * Note: NumPy's sinc is normalized, so we need to divide by Pi to get the
     * non-normalized output.
     *
     * >>> np.linspace(0,0.5,10)
     * array([ 0.        ,  0.05555556,  0.11111111,  0.16666667,  0.22222222,
     *         0.27777778,  0.33333333,  0.38888889,  0.44444444,  0.5       ])
     * >>> np.sinc(np.linspace(0,0.5,10) / np.pi)
     * array([ 1.        ,  0.99948568,  0.99794366,  0.9953768 ,  0.99178985,
     *         0.98718944,  0.98158409,  0.97498415,  0.96740182,  0.95885108])
     */
    const std::vector<Type> inputs =
    {
         0.        ,  0.05555556,  0.11111111,  0.16666667,  0.22222222,
         0.27777778,  0.33333333,  0.38888889,  0.44444444,  0.5
    };
    const std::vector<Type> expectedOutputs =
    {
        1.        ,  0.99948568,  0.99794366,  0.9953768 ,  0.99178985,
        0.98718944,  0.98158409,  0.97498415,  0.96740182,  0.95885108
    };

    const auto dtype = Pothos::DType(typeid(Type));
    std::cout << "Testing " << dtype.toString() << "..." << std::endl;

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto sinc = Pothos::BlockRegistry::make("/comms/sinc", dtype);
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    Pothos::BufferChunk inputBufferChunk(dtype, inputs.size());
    for(size_t i = 0; i < inputs.size(); ++i)
    {
        inputBufferChunk.as<Type*>()[i] = inputs[i];
    }
    feeder.call("feedBuffer", inputBufferChunk);

    {
        Pothos::Topology topology;

        topology.connect(
            feeder, 0,
            sinc, 0);
        topology.connect(
            sinc, 0,
            collector, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    const auto outputs = collector.call<Pothos::BufferChunk>("getBuffer");
    POTHOS_TEST_EQUAL(
        expectedOutputs.size(),
        outputs.elements());

    constexpr Type epsilon = 1e-3;
    for(size_t i = 0; i < expectedOutputs.size(); ++i)
    {
        POTHOS_TEST_CLOSE(
            expectedOutputs[i],
            outputs.as<const Type*>()[i],
            epsilon);
    }
}

POTHOS_TEST_BLOCK("/comms/tests", test_sinc)
{
    testSincTmpl<float>();
    testSincTmpl<double>();
}
