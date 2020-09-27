// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <type_traits>
#include <vector>

//
// Utility code
//

// Repeat the test values enough times to trigger both the SIMD and manual
// cases, if SIMD support is built in.
static constexpr size_t DefaultNumRepeats = 50;

template <typename Type>
static std::vector<Type> linspace(Type begin, Type end, Type step)
{
    POTHOS_TEST_LE(begin, end);

    std::vector<Type> output;
    output.emplace_back(begin);

    size_t multiplier = 0;
    while (output.back() <= end)
    {
        output.emplace_back(begin + (Type(++multiplier) * step));
    }
    if (output.back() > end) output.pop_back();

    return output;
}

template <typename Type>
static Pothos::BufferChunk processInputsForTest(
    const std::vector<Type>& input,
    size_t numRepeats = DefaultNumRepeats)
{
    std::vector<Type> outputVec;
    for (size_t i = 0; i < numRepeats; ++i)
    {
        outputVec.insert(outputVec.end(), input.begin(), input.end());
    }

    Pothos::BufferChunk output(typeid(Type), outputVec.size());
    std::memcpy(
        reinterpret_cast<void*>(output.address),
        outputVec.data(),
        output.length);

    return output;
}

//
// Test code
//

template <typename Type>
struct TestValues
{
    Pothos::BufferChunk inputs;
    Pothos::BufferChunk expectedOutputs;
    Type exponent;

    TestValues(const std::vector<Type>& vals, Type exp):
        inputs(processInputsForTest(vals)),
        expectedOutputs(processInputsForTest(vals)),
        exponent(exp)
    {
        // When the root can resolve to either a negative or positive
        // number, the block will return positive.
        if (std::is_signed<Type>::value && (std::fmod(exp,2) == 0))
        {
            for (size_t elem = 0; elem < expectedOutputs.elements(); ++elem)
            {
                if (inputs.as<const Type*>()[elem] < 0)
                {
                    expectedOutputs.as<Type*>()[elem] *= Type(-1);
                }
            }
        }
    }
};

template <typename Type>
static typename std::enable_if<sizeof(Type) == 1, std::vector<TestValues<Type>>>::type getTestValues()
{
    return std::vector<TestValues<Type>>
    {
        { {0, 1, 2, 3, 4, 5, 96, 97, 98, 99, 100}, 1 },
        { {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, 2 },
        { {0, 1, 2, 3, 4, 5}, 3 },
        { {0, 1, 2, 3}, 4 },
        { {0, 1, 2}, 5 },
        { {0, 1, 2}, 6 },
    };
}

template <typename Type>
static typename std::enable_if<sizeof(Type) == 2, std::vector<TestValues<Type>>>::type getTestValues()
{
    return std::vector<TestValues<Type>>
    {
        { {0, 1, 2, 3, 4, 5}, 1 },
        { {0, 1, 2, 3, 4, 5}, 2 },
        { {0, 1, 2, 3, 4, 5}, 3 },
        { {0, 1, 2, 3, 4, 5}, 4 },
        { {0, 1, 2, 3, 4, 5}, 5 },
        { {0, 1, 2, 3, 4, 5}, 6 },
        { {0, 1, 2, 3, 4}, 7 },
        { {0, 1, 2, 3}, 8 },
        { {0, 1, 2, 3}, 9 },
        { {0, 1, 2}, 10 },
    };
}

template <typename Type>
static typename std::enable_if<(sizeof(Type) > 2) && std::is_unsigned<Type>::value, std::vector<TestValues<Type>>>::type getTestValues()
{
    const auto inputs = linspace<Type>(0, 5, 1);

    return std::vector<TestValues<Type>>
    {
        { inputs, 1 },
        { inputs, 2 },
        { inputs, 3 },
        { inputs, 4 },
        { inputs, 5 },
        { inputs, 6 },
        { inputs, 7 },
        { inputs, 8 },
        { inputs, 9 },
        { inputs, 10 },
    };
}

template <typename Type>
static typename std::enable_if<(sizeof(Type) > 2) && std::is_signed<Type>::value && !std::is_floating_point<Type>::value, std::vector<TestValues<Type>>>::type getTestValues()
{
    const auto inputs = linspace<Type>(-8, 8, 1);

    return std::vector<TestValues<Type>>
    {
        { inputs, 1 },
        { inputs, 2 },
        { inputs, 3 },
        { inputs, 4 },
        { inputs, 5 },
        { inputs, 6 },
        { inputs, 7 },
        { inputs, 8 },
        { inputs, 9 },
        { inputs, 10 },
    };
}

template <typename Type>
static typename std::enable_if<std::is_floating_point<Type>::value, std::vector<TestValues<Type>>>::type getTestValues()
{
    const auto inputs = linspace<Type>(-10, 10, Type(0.1));

    return std::vector<TestValues<Type>>
    {
        { inputs, -10 },
        { inputs, -8 },
        { inputs, -6 },
        { inputs, -4 },
        { inputs, -2 },
        { inputs, 1 },
        { inputs, 2 },
        { inputs, 3 },
        { inputs, 4 },
        { inputs, 5 },
        { inputs, 6 },
        { inputs, 7 },
        { inputs, 8 },
        { inputs, 9 },
        { inputs, 10 },
    };
}

template <typename Type>
static typename std::enable_if<!std::is_floating_point<Type>::value, void>::type compareBufferChunks(
    const Pothos::BufferChunk& expected,
    const Pothos::BufferChunk& output)
{
    POTHOS_TEST_EQUAL(expected.dtype, output.dtype);
    POTHOS_TEST_EQUAL(expected.elements(), output.elements());

    POTHOS_TEST_EQUALA(
        expected.as<const Type*>(),
        output.as<const Type*>(),
        expected.elements());
}

template <typename Type>
static typename std::enable_if<std::is_floating_point<Type>::value, void>::type compareBufferChunks(
    const Pothos::BufferChunk& expected,
    const Pothos::BufferChunk& output)
{
    static constexpr Type epsilon = Type(1e-6);

    POTHOS_TEST_EQUAL(expected.dtype, output.dtype);
    POTHOS_TEST_EQUAL(expected.elements(), output.elements());

    POTHOS_TEST_CLOSEA(
        expected.as<const Type*>(),
        output.as<const Type*>(),
        epsilon,
        expected.elements());
}

//
// Test implementation
//

template <typename Type>
static void testPowRoot(
    const TestValues<Type>& testValues,
    const Pothos::Proxy& root)
{
    static const auto dtype = Pothos::DType(typeid(Type));

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    feeder.call("feedBuffer", testValues.inputs);

    auto pow = Pothos::BlockRegistry::make("/comms/pow", dtype, testValues.exponent);
    POTHOS_TEST_EQUAL(
        testValues.exponent,
        pow.template call<Type>("exponent"));

    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;

        topology.connect(feeder, 0, pow, 0);
        topology.connect(pow, 0, root, 0);
        topology.connect(root, 0, collector, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    compareBufferChunks<Type>(
        testValues.expectedOutputs,
        collector.call<Pothos::BufferChunk>("getBuffer"));
}

template <typename Type>
static void testPowRoot()
{
    static const auto dtype = Pothos::DType(typeid(Type));

    std::cout << "Testing " << dtype.toString() << "..." << std::endl;

    auto allTestValues = getTestValues<Type>();

    for (const auto& testValues : allTestValues)
    {
        // If applicable, test the specialized blocks.

        if (testValues.exponent == Type(2))
        {
            std::cout << " * Testing /comms/pow(2) -> /comms/sqrt..." << std::endl;
            testPowRoot<Type>(testValues, Pothos::BlockRegistry::make("/comms/sqrt", dtype));
        }
        else if(testValues.exponent == Type(3))
        {
            std::cout << " * Testing /comms/pow(3) -> /comms/cbrt..." << std::endl;
            testPowRoot<Type>(testValues, Pothos::BlockRegistry::make("/comms/cbrt", dtype));
        }

        // Test Nth Root.
        std::cout << " * Testing /comms/pow(" << Pothos::Object(testValues.exponent).toString()
                  << ") -> /comms/nth_root(" << Pothos::Object(testValues.exponent).toString() << ")..." << std::endl;
        auto nthRoot = Pothos::BlockRegistry::make("/comms/nth_root", dtype, testValues.exponent);
        POTHOS_TEST_EQUAL(
            testValues.exponent,
            nthRoot.template call<Type>("root"));
        testPowRoot<Type>(testValues, nthRoot);
    }
}

POTHOS_TEST_BLOCK("/comms/tests", test_pow_root)
{
    testPowRoot<std::int8_t>();
    testPowRoot<std::int16_t>();
    testPowRoot<std::int32_t>();
    testPowRoot<std::int64_t>();
    testPowRoot<std::uint8_t>();
    testPowRoot<std::uint16_t>();
    testPowRoot<std::uint32_t>();
    testPowRoot<std::uint64_t>();
    testPowRoot<float>();
    testPowRoot<double>();
}
