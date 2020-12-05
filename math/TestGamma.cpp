// Copyright (c) 2020 Nicholas Corgan
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
    Pothos::BufferChunk* pGammaOutput,
    Pothos::BufferChunk* pLnGammaOutput)
{
    // np.linspace(1,2,100)
    *pInput = CommsTests::stdVectorToBufferChunk<T>(
    {
        1.0f       , 1.01010101f, 1.02020202f, 1.03030303f, 1.04040404f,
        1.05050505f, 1.06060606f, 1.07070707f, 1.08080808f, 1.09090909f,
        1.1010101f , 1.11111111f, 1.12121212f, 1.13131313f, 1.14141414f,
        1.15151515f, 1.16161616f, 1.17171717f, 1.18181818f, 1.19191919f,
        1.2020202f , 1.21212121f, 1.22222222f, 1.23232323f, 1.24242424f,
        1.25252525f, 1.26262626f, 1.27272727f, 1.28282828f, 1.29292929f,
        1.3030303f , 1.31313131f, 1.32323232f, 1.33333333f, 1.34343434f,
        1.35353535f, 1.36363636f, 1.37373737f, 1.38383838f, 1.39393939f,
        1.4040404f , 1.41414141f, 1.42424242f, 1.43434343f, 1.44444444f,
        1.45454545f, 1.46464646f, 1.47474747f, 1.48484848f, 1.49494949f,
        1.50505051f, 1.51515152f, 1.52525253f, 1.53535354f, 1.54545455f,
        1.55555556f, 1.56565657f, 1.57575758f, 1.58585859f, 1.5959596f ,
        1.60606061f, 1.61616162f, 1.62626263f, 1.63636364f, 1.64646465f,
        1.65656566f, 1.66666667f, 1.67676768f, 1.68686869f, 1.6969697f ,
        1.70707071f, 1.71717172f, 1.72727273f, 1.73737374f, 1.74747475f,
        1.75757576f, 1.76767677f, 1.77777778f, 1.78787879f, 1.7979798f ,
        1.80808081f, 1.81818182f, 1.82828283f, 1.83838384f, 1.84848485f,
        1.85858586f, 1.86868687f, 1.87878788f, 1.88888889f, 1.8989899f ,
        1.90909091f, 1.91919192f, 1.92929293f, 1.93939394f, 1.94949495f,
        1.95959596f, 1.96969697f, 1.97979798f, 1.98989899f, 2.
    });

    // scipy.special.gamma(np.linspace(1,2,100))
    *pGammaOutput = CommsTests::stdVectorToBufferChunk<T>(
    {
        1.0f       , 0.99426953f, 0.98873541f, 0.98339239f, 0.97823543f,
        0.97325971f, 0.9684606f , 0.96383367f, 0.95937467f, 0.95507953f,
        0.95094434f, 0.94696535f, 0.94313896f, 0.93946173f, 0.93593033f,
        0.93254159f, 0.92929247f, 0.92618001f, 0.92320142f, 0.920354f  ,
        0.91763513f, 0.91504235f, 0.91257324f, 0.91022551f, 0.90799696f,
        0.90588546f, 0.90388899f, 0.90200558f, 0.90023336f, 0.89857052f,
        0.89701535f, 0.89556617f, 0.8942214f , 0.89297951f, 0.89183904f,
        0.89079857f, 0.88985677f, 0.88901234f, 0.88826405f, 0.88761071f,
        0.8870512f , 0.88658443f, 0.88620937f, 0.88592502f, 0.88573045f,
        0.88562476f, 0.88560708f, 0.88567661f, 0.88583256f, 0.8860742f ,
        0.88640082f, 0.88681176f, 0.8873064f , 0.88788415f, 0.88854443f,
        0.88928673f, 0.89011056f, 0.89101544f, 0.89200094f, 0.89306668f,
        0.89421226f, 0.89543735f, 0.89674164f, 0.89812482f, 0.89958664f,
        0.90112687f, 0.90274529f, 0.90444172f, 0.906216f  , 0.908068f  ,
        0.90999759f, 0.91200471f, 0.91408927f, 0.91625124f, 0.9184906f ,
        0.92080735f, 0.92320151f, 0.92567315f, 0.92822231f, 0.93084909f,
        0.9335536f , 0.93633598f, 0.93919636f, 0.94213493f, 0.94515186f,
        0.94824738f, 0.95142172f, 0.95467512f, 0.95800785f, 0.96142021f,
        0.96491249f, 0.96848503f, 0.97213817f, 0.97587228f, 0.97968774f,
        0.98358495f, 0.98756433f, 0.99162632f, 0.99577139f, 1.
    });

    // scipy.special.gammaln(np.linspace(1,2,100))
    *pLnGammaOutput = CommsTests::stdVectorToBufferChunk<T>(
    {
         0.0f       , -0.00574695f, -0.01132852f, -0.01674706f, -0.02200491f,
        -0.02710431f, -0.03204748f, -0.03683654f, -0.04147359f, -0.04596066f,
        -0.05029975f, -0.05449278f, -0.05854165f, -0.0624482f , -0.06621424f,
        -0.06984152f, -0.07333177f, -0.07668666f, -0.07990784f, -0.0829969f ,
        -0.08595542f, -0.08878494f, -0.09148694f, -0.0940629f , -0.09651425f,
        -0.0988424f , -0.10104873f, -0.10313457f, -0.10510126f, -0.10695009f,
        -0.10868231f, -0.11029917f, -0.11180188f, -0.11319164f, -0.11446962f,
        -0.11563695f, -0.11669477f, -0.11764416f, -0.11848623f, -0.11922202f,
        -0.11985257f, -0.12037892f, -0.12080205f, -0.12112295f, -0.1213426f ,
        -0.12146194f, -0.1214819f , -0.1214034f , -0.12122733f, -0.12095459f,
        -0.12058604f, -0.12012254f, -0.11956492f, -0.11891401f, -0.11817063f,
        -0.11733556f, -0.1164096f , -0.11539353f, -0.11428809f, -0.11309403f,
        -0.1118121f , -0.11044302f, -0.10898749f, -0.10744622f, -0.10581991f,
        -0.10410922f, -0.10231483f, -0.10043741f, -0.09847759f, -0.09643602f,
        -0.09431332f, -0.09211013f, -0.08982705f, -0.08746468f, -0.08502361f,
        -0.08250444f, -0.07990774f, -0.07723408f, -0.07448402f, -0.07165811f,
        -0.0687569f , -0.06578092f, -0.06273071f, -0.05960678f, -0.05640966f,
        -0.05313986f, -0.04979787f, -0.04638419f, -0.04289931f, -0.03934371f,
        -0.03571787f, -0.03202225f, -0.02825733f, -0.02442356f, -0.0205214f ,
        -0.01655127f, -0.01251364f, -0.00840893f, -0.00423758f,  0.
    });
}

template <typename T>
static void testGamma()
{
    const Pothos::DType dtype(typeid(T));
    std::cout << "Testing " << dtype.toString() << "..." << std::endl;

    Pothos::BufferChunk input, gammaOutput, lnGammaOutput;
    getTestInputs<T>(&input, &gammaOutput, &lnGammaOutput);

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    feeder.call("feedBuffer", input);

    auto gamma = Pothos::BlockRegistry::make("/comms/gamma", dtype);
    auto lnGamma = Pothos::BlockRegistry::make("/comms/lngamma", dtype);

    auto gammaCollector = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    auto lnGammaCollector = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;

        topology.connect(feeder, 0, gamma, 0);
        topology.connect(gamma, 0, gammaCollector, 0);

        topology.connect(feeder, 0, lnGamma, 0);
        topology.connect(lnGamma, 0, lnGammaCollector, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    constexpr T epsilon = T(1e-6);

    std::cout << " * Testing /comms/gamma..." << std::endl;
    CommsTests::testBufferChunksClose<T>(
        gammaOutput,
        gammaCollector.call<Pothos::BufferChunk>("getBuffer"),
        epsilon);

    std::cout << " * Testing /comms/lngamma..." << std::endl;
    CommsTests::testBufferChunksClose<T>(
        lnGammaOutput,
        lnGammaCollector.call<Pothos::BufferChunk>("getBuffer"),
        epsilon);
}

POTHOS_TEST_BLOCK("/comms/tests", test_gamma)
{
    testGamma<float>();
    testGamma<double>();
}
