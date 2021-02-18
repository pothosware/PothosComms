// Copyright (c) 2021 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "common/Testing.hpp"

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>

#include <cstring>
#include <iostream>
#include <vector>

template <typename T>
static void getTestInputs(
    Pothos::BufferChunk* pInput,
    Pothos::BufferChunk* pErfOutput,
    Pothos::BufferChunk* pErfcOutput)
{
    // np.linspace(0,1,101)
    *pInput = CommsTests::stdVectorToBufferChunk<T>(
    {
        0.0f , 0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f, 0.07f, 0.08f, 0.09f, 0.1f,
        0.11f, 0.12f, 0.13f, 0.14f, 0.15f, 0.16f, 0.17f, 0.18f, 0.19f, 0.2f , 0.21f,
        0.22f, 0.23f, 0.24f, 0.25f, 0.26f, 0.27f, 0.28f, 0.29f, 0.3f , 0.31f, 0.32f,
        0.33f, 0.34f, 0.35f, 0.36f, 0.37f, 0.38f, 0.39f, 0.4f , 0.41f, 0.42f, 0.43f,
        0.44f, 0.45f, 0.46f, 0.47f, 0.48f, 0.49f, 0.5f , 0.51f, 0.52f, 0.53f, 0.54f,
        0.55f, 0.56f, 0.57f, 0.58f, 0.59f, 0.6f , 0.61f, 0.62f, 0.63f, 0.64f, 0.65f,
        0.66f, 0.67f, 0.68f, 0.69f, 0.7f , 0.71f, 0.72f, 0.73f, 0.74f, 0.75f, 0.76f,
        0.77f, 0.78f, 0.79f, 0.8f , 0.81f, 0.82f, 0.83f, 0.84f, 0.85f, 0.86f, 0.87f,
        0.88f, 0.89f, 0.9f , 0.91f, 0.92f, 0.93f, 0.94f, 0.95f, 0.96f, 0.97f, 0.98f,
        0.99f, 1.0f
    });

    // Source: Wolfram Alpha
    *pErfOutput = CommsTests::stdVectorToBufferChunk<T>(
    {
        0.0f, 0.0112834f, 0.0225646f, 0.0338412f, 0.0451111f, 0.056372f, 0.0676216f, 0.0788577f, 0.0900781f, 0.101281f, 0.112463f,
        0.123623f, 0.134758f, 0.145867f, 0.156947f, 0.167996f, 0.179012f, 0.189992f, 0.200936f, 0.21184f, 0.222703f, 0.233522f,
        0.244296f, 0.255023f, 0.2657f, 0.276326f, 0.2869f, 0.297418f, 0.30788f, 0.318283f, 0.328627f, 0.338908f, 0.349126f,
        0.359279f, 0.369365f, 0.379382f, 0.38933f, 0.399206f, 0.409009f, 0.418739f, 0.428392f, 0.437969f, 0.447468f, 0.456887f,
        0.466225f, 0.475482f, 0.484655f, 0.493745f, 0.50275f, 0.511668f, 0.5205f, 0.529244f, 0.537899f, 0.546464f, 0.554939f,
        0.563323f, 0.571616f, 0.579816f, 0.587923f, 0.595936f, 0.603856f, 0.611681f, 0.619411f, 0.627046f, 0.634586f, 0.642029f,
        0.649377f, 0.656628f, 0.663782f, 0.67084f, 0.677801f, 0.684666f, 0.691433f, 0.698104f, 0.704678f, 0.711156f, 0.717537f,
        0.723822f, 0.73001f, 0.736103f, 0.742101f, 0.748003f, 0.753811f, 0.759524f, 0.765143f, 0.770668f, 0.7761f, 0.78144f,
        0.786687f, 0.791843f, 0.796908f, 0.801883f, 0.806768f, 0.811564f, 0.816271f, 0.820891f, 0.825424f, 0.82987f, 0.834232f,
        0.838508f, 0.842701f
    });

    // Source: Wolfram Alpha
    *pErfcOutput = CommsTests::stdVectorToBufferChunk<T>(
    {
        1.0f, 0.988717f, 0.977435f, 0.966159f, 0.954889f, 0.943628f, 0.932378f, 0.921142f, 0.909922f, 0.898719f, 0.887537f,
        0.876377f, 0.865242f, 0.854133f, 0.843053f, 0.832004f, 0.820988f, 0.810008f, 0.799064f, 0.78816f, 0.777297f, 0.766478f,
        0.755704f, 0.744977f, 0.7343f, 0.723674f, 0.7131f, 0.702582f, 0.69212f, 0.681717f, 0.671373f, 0.661092f, 0.650874f,
        0.640721f, 0.630635f, 0.620618f, 0.61067f, 0.600794f, 0.590991f, 0.581261f, 0.571608f, 0.562031f, 0.552532f, 0.543113f,
        0.533775f, 0.524518f, 0.515345f, 0.506255f, 0.49725f, 0.488332f, 0.4795f, 0.470756f, 0.462101f, 0.453536f, 0.445061f,
        0.436677f, 0.428384f, 0.420184f, 0.412077f, 0.404064f, 0.396144f, 0.388319f, 0.380589f, 0.372954f, 0.365414f, 0.357971f,
        0.350623f, 0.343372f, 0.336218f, 0.32916f, 0.322199f, 0.315334f, 0.308567f, 0.301896f, 0.295322f, 0.288844f, 0.282463f,
        0.276178f, 0.26999f, 0.263897f, 0.257899f, 0.251997f, 0.246189f, 0.240476f, 0.234857f, 0.229332f, 0.2239f, 0.21856f,
        0.213313f, 0.208157f, 0.203092f, 0.198117f, 0.193232f, 0.188436f, 0.183729f, 0.179109f, 0.174576f, 0.17013f, 0.165768f,
        0.161492f, 0.157299f
    });
}

template <typename T>
static void testErf()
{
    const Pothos::DType dtype(typeid(T));
    std::cout << "Testing " << dtype.toString() << "..." << std::endl;

    Pothos::BufferChunk input, erfOutput, erfcOutput;
    getTestInputs<T>(&input, &erfOutput, &erfcOutput);

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    feeder.call("feedBuffer", input);

    auto erf = Pothos::BlockRegistry::make("/comms/erf", dtype);
    auto erfc = Pothos::BlockRegistry::make("/comms/erfc", dtype);

    auto erfCollector = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    auto erfcCollector = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;

        topology.connect(feeder, 0, erf, 0);
        topology.connect(erf, 0, erfCollector, 0);

        topology.connect(feeder, 0, erfc, 0);
        topology.connect(erfc, 0, erfcCollector, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    constexpr T epsilon = T(1e-6);

    std::cout << " * Testing /comms/erf..." << std::endl;
    CommsTests::testBufferChunksClose<T>(
        erfOutput,
        erfCollector.call<Pothos::BufferChunk>("getBuffer"),
        epsilon);

    std::cout << " * Testing /comms/erfc..." << std::endl;
    CommsTests::testBufferChunksClose<T>(
        erfcOutput,
        erfcCollector.call<Pothos::BufferChunk>("getBuffer"),
        epsilon);
}

POTHOS_TEST_BLOCK("/comms/tests", test_erf)
{
    testErf<float>();
    testErf<double>();
}
