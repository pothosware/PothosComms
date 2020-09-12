// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

#define _USE_MATH_DEFINES
#include <cmath>

//
// Utility code
//

constexpr size_t bufferLen = 123; // Long enough for any SIMD frame, while having manual entries

// https://gist.github.com/lorenzoriano/5414671
template <typename T>
static std::vector<T> linspace(T a, T b, size_t N)
{
    T h = (b - a) / static_cast<T>(N - 1);
    std::vector<T> xs(N);
    typename std::vector<T>::iterator x;
    T val;
    for (x = xs.begin(), val = a; x != xs.end(); ++x, val += h)
        *x = val;
    return xs;
}

//
// To be used when we want both sides of a noncontinuous domain
//
// ----------------|   |----------------
//
template <typename T>
std::vector<T> linspaceOutsideRange(T leftMin, T leftMax, T rightMin, T rightMax, size_t N)
{
    auto output = linspace<T>(leftMin, leftMax, N / 2);
    auto rightLinSpace = linspace<T>(rightMin, rightMax, N / 2);

    output.insert(output.end(), rightLinSpace.begin(), rightLinSpace.end());
    return output;
}

template <typename T>
static Pothos::BufferChunk stdVectorToBufferChunk(const std::vector<T>& stdVector)
{
    static const Pothos::DType dtype(typeid(T));

    Pothos::BufferChunk bufferChunk(dtype, stdVector.size());
    std::memcpy(
        reinterpret_cast<void*>(bufferChunk.address),
        stdVector.data(),
        stdVector.size() * sizeof(T));

    return bufferChunk;
}

//
// Test implementation
//

template <typename T>
struct TestParams
{
    Pothos::BufferChunk inputs;
    Pothos::BufferChunk expectedOutputs;

    TestParams(const std::vector<T>& inputVec)
    {
        static const Pothos::DType dtype(typeid(T));

        inputs = stdVectorToBufferChunk(inputVec);
        expectedOutputs = Pothos::BufferChunk(dtype, inputVec.size());
    }
};

template <typename T>
static void testTrigonometricOperation(
    const std::string& operation,
    const TestParams<T>& testParams)
{
    std::cout << " * Testing " << operation << "..." << std::endl;

    static const Pothos::DType dtype(typeid(T));

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    feeder.call("feedBuffer", testParams.inputs);

    auto trig = Pothos::BlockRegistry::make("/comms/trigonometric", dtype, operation);
    auto sink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;

        topology.connect(feeder, 0, trig, 0);
        topology.connect(trig, 0, sink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    static constexpr T epsilon = T(1e-6);

    auto outputs = sink.call<Pothos::BufferChunk>("getBuffer");
    POTHOS_TEST_EQUAL(testParams.expectedOutputs.dtype, outputs.dtype);
    POTHOS_TEST_EQUAL(testParams.expectedOutputs.elements(), outputs.elements());
    POTHOS_TEST_CLOSEA(
        testParams.expectedOutputs.as<const T*>(),
        outputs.as<const T*>(),
        epsilon,
        outputs.elements());
}

template <typename T>
static void testTrigonmetricBlockForType()
{
    std::cout << "Testing " << Pothos::DType(typeid(T)).name() << "..." << std::endl;

    // Slightly offset from bounds of domain with real values
    const auto piDiv2Inputs = linspace<T>(T(-M_PI/2 + 0.01), T(M_PI/2 - 0.01), bufferLen);
    const auto oneToOneInputs = linspace<T>(T(-0.99), T(0.99), bufferLen);
    const auto oneToPiInputs = linspace<T>(T(1.01), T(M_PI), bufferLen);
    const auto zeroToOneInputs = linspace<T>(T(0.01), T(0.99), bufferLen);

    const auto outsideOneToOneInputs = linspaceOutsideRange<T>(
                                           T(-M_PI / 2 + 0.01), T(-1.01),
                                           T(1.01), T(M_PI / 2 + 0.01),
                                           bufferLen);

    TestParams<T> cosTestParams(piDiv2Inputs);
    TestParams<T> sinTestParams(piDiv2Inputs);
    TestParams<T> tanTestParams(piDiv2Inputs);
    TestParams<T> secTestParams(piDiv2Inputs);
    TestParams<T> cscTestParams(piDiv2Inputs);
    TestParams<T> cotTestParams(piDiv2Inputs);

    TestParams<T> acosTestParams(oneToOneInputs);
    TestParams<T> asinTestParams(oneToOneInputs);
    TestParams<T> atanTestParams(oneToOneInputs);
    TestParams<T> asecTestParams(outsideOneToOneInputs);
    TestParams<T> acscTestParams(outsideOneToOneInputs);
    TestParams<T> acotTestParams(piDiv2Inputs);

    TestParams<T> coshTestParams(piDiv2Inputs);
    TestParams<T> sinhTestParams(piDiv2Inputs);
    TestParams<T> tanhTestParams(piDiv2Inputs);
    TestParams<T> sechTestParams(piDiv2Inputs);
    TestParams<T> cschTestParams(piDiv2Inputs);
    TestParams<T> cothTestParams(piDiv2Inputs);

    TestParams<T> acoshTestParams(oneToPiInputs);
    TestParams<T> asinhTestParams(piDiv2Inputs);
    TestParams<T> atanhTestParams(oneToOneInputs);
    TestParams<T> asechTestParams(zeroToOneInputs);
    TestParams<T> acschTestParams(outsideOneToOneInputs);
    TestParams<T> acothTestParams(outsideOneToOneInputs);


    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        cosTestParams.expectedOutputs.as<T*>()[elem] = std::cos(piDiv2Inputs[elem]);
        sinTestParams.expectedOutputs.as<T*>()[elem] = std::sin(piDiv2Inputs[elem]);
        tanTestParams.expectedOutputs.as<T*>()[elem] = std::tan(piDiv2Inputs[elem]);
        secTestParams.expectedOutputs.as<T*>()[elem] = T(1.0) / std::cos(piDiv2Inputs[elem]);
        cscTestParams.expectedOutputs.as<T*>()[elem] = T(1.0) / std::sin(piDiv2Inputs[elem]);
        cotTestParams.expectedOutputs.as<T*>()[elem] = T(1.0) / std::tan(piDiv2Inputs[elem]);

        acosTestParams.expectedOutputs.as<T*>()[elem] = std::acos(oneToOneInputs[elem]);
        asinTestParams.expectedOutputs.as<T*>()[elem] = std::asin(oneToOneInputs[elem]);
        atanTestParams.expectedOutputs.as<T*>()[elem] = std::atan(oneToOneInputs[elem]);
        asecTestParams.expectedOutputs.as<T*>()[elem] = std::acos(T(1.0) / outsideOneToOneInputs[elem]);
        acscTestParams.expectedOutputs.as<T*>()[elem] = std::asin(T(1.0) / outsideOneToOneInputs[elem]);
        acotTestParams.expectedOutputs.as<T*>()[elem] = std::atan(T(1.0) / piDiv2Inputs[elem]);

        coshTestParams.expectedOutputs.as<T*>()[elem] = std::cosh(piDiv2Inputs[elem]);
        sinhTestParams.expectedOutputs.as<T*>()[elem] = std::sinh(piDiv2Inputs[elem]);
        tanhTestParams.expectedOutputs.as<T*>()[elem] = std::tanh(piDiv2Inputs[elem]);
        sechTestParams.expectedOutputs.as<T*>()[elem] = T(1.0) / std::cosh(piDiv2Inputs[elem]);
        cschTestParams.expectedOutputs.as<T*>()[elem] = T(1.0) / std::sinh(piDiv2Inputs[elem]);
        cothTestParams.expectedOutputs.as<T*>()[elem] = T(1.0) / std::tanh(piDiv2Inputs[elem]);

        acoshTestParams.expectedOutputs.as<T*>()[elem] = std::acosh(oneToPiInputs[elem]);
        asinhTestParams.expectedOutputs.as<T*>()[elem] = std::asinh(piDiv2Inputs[elem]);
        atanhTestParams.expectedOutputs.as<T*>()[elem] = std::atanh(oneToOneInputs[elem]);
        asechTestParams.expectedOutputs.as<T*>()[elem] = std::acosh(T(1.0) / zeroToOneInputs[elem]);
        acschTestParams.expectedOutputs.as<T*>()[elem] = std::asinh(T(1.0) / outsideOneToOneInputs[elem]);
        acothTestParams.expectedOutputs.as<T*>()[elem] = std::atanh(T(1.0) / outsideOneToOneInputs[elem]);
    }

    testTrigonometricOperation<T>("COS", cosTestParams);
    testTrigonometricOperation<T>("SIN", sinTestParams);
    testTrigonometricOperation<T>("TAN", tanTestParams);
    testTrigonometricOperation<T>("SEC", secTestParams);
    testTrigonometricOperation<T>("CSC", cscTestParams);
    testTrigonometricOperation<T>("COT", cotTestParams);

    testTrigonometricOperation<T>("ACOS", acosTestParams);
    testTrigonometricOperation<T>("ASIN", asinTestParams);
    testTrigonometricOperation<T>("ATAN", atanTestParams);
    testTrigonometricOperation<T>("ASEC", asecTestParams);
    testTrigonometricOperation<T>("ACSC", acscTestParams);
    testTrigonometricOperation<T>("ACOT", acotTestParams);

    testTrigonometricOperation<T>("COSH", coshTestParams);
    testTrigonometricOperation<T>("SINH", sinhTestParams);
    testTrigonometricOperation<T>("TANH", tanhTestParams);
    testTrigonometricOperation<T>("SECH", sechTestParams);
    testTrigonometricOperation<T>("CSCH", cschTestParams);
    testTrigonometricOperation<T>("COTH", cothTestParams);

    testTrigonometricOperation<T>("ACOSH", acoshTestParams);
    testTrigonometricOperation<T>("ASINH", asinhTestParams);
    testTrigonometricOperation<T>("ATANH", atanhTestParams);
    testTrigonometricOperation<T>("ASECH", asechTestParams);
    testTrigonometricOperation<T>("ACSCH", acschTestParams);
    testTrigonometricOperation<T>("ACOTH", acothTestParams);
}

POTHOS_TEST_BLOCK("/comms/tests", test_trigonometric)
{
    testTrigonmetricBlockForType<float>();
    testTrigonmetricBlockForType<double>();
}