// Copyright (c) 2014-2016 Josh Blum
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>

#include <algorithm>
#include <complex>
#include <cstring>
#include <iostream>
#include <type_traits>
#include <vector>

//
// Common
//

constexpr size_t bufferLen = 64; // Long enough for any SIMD frame

template <typename T>
struct IsComplex : std::false_type {};

template <typename T>
struct IsComplex<std::complex<T>> : std::true_type {};

//
// /comms/arithmetic
//

struct ArithmeticTestValues
{
    std::vector<Pothos::BufferChunk> inputs;
    Pothos::BufferChunk expectedOutputs;

    template <typename T>
    void setup(size_t numInputs, size_t bufferLength)
    {
        static const Pothos::DType dtype(typeid(T));

        for (size_t input = 0; input < numInputs; ++input)
        {
            inputs.emplace_back(Pothos::BufferChunk(dtype, bufferLength));
        }

        expectedOutputs = Pothos::BufferChunk(dtype, bufferLength);
    }
};

template <typename T>
static typename std::enable_if<!IsComplex<T>::value, ArithmeticTestValues>::type getAddTestValues()
{
    constexpr auto numInputs = 3;

    static const Pothos::DType dtype(typeid(T));

    ArithmeticTestValues testValues;
    testValues.setup<T>(numInputs, bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        testValues.inputs[0].as<T*>()[elem] = static_cast<T>(elem);
        testValues.inputs[1].as<T*>()[elem] = static_cast<T>(elem/2);
        testValues.inputs[2].as<T*>()[elem] = static_cast<T>(elem/4);

        if (std::is_signed<T>::value)
        {
            testValues.inputs[1].as<T*>()[elem] *= -1;
            testValues.inputs[2].as<T*>()[elem] *= -1;

            testValues.expectedOutputs.as<T*>()[elem] = static_cast<T>(elem - (elem / 2) - (elem / 4));
        }
        else
        {
            testValues.expectedOutputs.as<T*>()[elem] = static_cast<T>(elem + (elem / 2) + (elem / 4));
        }
    }

    return testValues;
}

// Fully co-opt the scalar implementation since complex addition is just (real+real, imag+imag)
template <typename T>
static typename std::enable_if<IsComplex<T>::value, ArithmeticTestValues>::type getAddTestValues()
{
    using ScalarType = typename T::value_type;
    static const Pothos::DType dtype(typeid(T));

    auto testValues = getAddTestValues<ScalarType>();
    for (auto& input : testValues.inputs) input.dtype = dtype;
    testValues.expectedOutputs.dtype = dtype;

    return testValues;
}

template <typename T>
static typename std::enable_if<!IsComplex<T>::value, ArithmeticTestValues>::type getSubTestValues()
{
    constexpr auto numInputs = 2;

    static const Pothos::DType dtype(typeid(T));

    ArithmeticTestValues testValues;
    testValues.setup<T>(numInputs, bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        testValues.inputs[0].as<T*>()[elem] = static_cast<T>(elem);
        testValues.inputs[1].as<T*>()[elem] = std::is_signed<T>::value ? static_cast<T>(elem * 2)
                                                                       : static_cast<T>(elem / 2);

        testValues.expectedOutputs.as<T*>()[elem] = testValues.inputs[0].as<T*>()[elem]
                                                  - testValues.inputs[1].as<T*>()[elem];
    }

    return testValues;
}

// Fully co-opt the scalar implementation since complex subtraction is just (real-real, imag-imag)
template <typename T>
static typename std::enable_if<IsComplex<T>::value, ArithmeticTestValues>::type getSubTestValues()
{
    using ScalarType = typename T::value_type;
    static const Pothos::DType dtype(typeid(T));

    auto testValues = getSubTestValues<ScalarType>();
    for (auto& input : testValues.inputs) input.dtype = dtype;
    testValues.expectedOutputs.dtype = dtype;

    return testValues;
}

template <typename T>
static typename std::enable_if<!IsComplex<T>::value, ArithmeticTestValues>::type getMulTestValues()
{
    constexpr auto numInputs = 2;

    static const Pothos::DType dtype(typeid(T));

    ArithmeticTestValues testValues;
    testValues.setup<T>(numInputs, bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        testValues.inputs[0].as<T*>()[elem] = static_cast<T>(elem);
        testValues.inputs[1].as<T*>()[elem] = static_cast<T>((elem % 2) + 1);

        if (std::is_signed<T>::value) testValues.inputs[1].as<T*>()[elem] *= -1;

        testValues.expectedOutputs.as<T*>()[elem] = testValues.inputs[0].as<T*>()[elem]
                                                  * testValues.inputs[1].as<T*>()[elem];
    }

    return testValues;
}

// Out of laziness, get the scalar version's values and recalculate the outputs.
template <typename T>
static typename std::enable_if<IsComplex<T>::value, ArithmeticTestValues>::type getMulTestValues()
{
    using ScalarType = typename T::value_type;
    static const Pothos::DType dtype(typeid(T));

    auto testValues = getSubTestValues<ScalarType>();
    POTHOS_TEST_EQUAL(2, testValues.inputs.size());

    for (auto& input : testValues.inputs) input.dtype = dtype;
    testValues.expectedOutputs.dtype = dtype;

    for (size_t elem = 0; elem < testValues.expectedOutputs.elements(); ++elem)
    {
        testValues.expectedOutputs.as<T*>()[elem] = testValues.inputs[0].as<const T*>()[elem]
                                                  * testValues.inputs[1].as<const T*>()[elem];
    }

    return testValues;
}

template <typename T>
static typename std::enable_if<!IsComplex<T>::value, ArithmeticTestValues>::type getDivTestValues()
{
    constexpr auto numInputs = 2;

    static const Pothos::DType dtype(typeid(T));

    ArithmeticTestValues testValues;
    testValues.setup<T>(numInputs, bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        testValues.inputs[0].as<T*>()[elem] = static_cast<T>(elem);
        testValues.inputs[1].as<T*>()[elem] = static_cast<T>((elem % 2) + 1);

        if (std::is_signed<T>::value) testValues.inputs[1].as<T*>()[elem] *= -1;

        testValues.expectedOutputs.as<T*>()[elem] = testValues.inputs[0].as<T*>()[elem]
                                                  / testValues.inputs[1].as<T*>()[elem];
    }

    return testValues;
}

// Out of laziness, get the scalar version's values and recalculate the outputs.
template <typename T>
static typename std::enable_if<IsComplex<T>::value, ArithmeticTestValues>::type getDivTestValues()
{
    using ScalarType = typename T::value_type;
    static const Pothos::DType dtype(typeid(T));

    auto testValues = getSubTestValues<ScalarType>();
    POTHOS_TEST_EQUAL(2, testValues.inputs.size());

    for (auto& input : testValues.inputs) input.dtype = dtype;
    testValues.expectedOutputs.dtype = dtype;

    for (size_t elem = 0; elem < testValues.expectedOutputs.elements(); ++elem)
    {
        testValues.expectedOutputs.as<T*>()[elem] = testValues.inputs[0].as<const T*>()[elem]
                                                  / testValues.inputs[1].as<const T*>()[elem];
    }

    return testValues;
}

template <typename T>
static void testArithmeticOp(
    const std::string& operation,
    const ArithmeticTestValues& testValues)
{
    const Pothos::DType dtype(typeid(T));

    std::cout << " * Testing " << operation << "..." << std::endl;

    const auto numInputs = testValues.inputs.size();

    auto arithmetic = Pothos::BlockRegistry::make("/comms/arithmetic", dtype, operation);
    arithmetic.call("setNumInputs", numInputs);

    std::vector<Pothos::Proxy> feeders(numInputs);
    for (size_t input = 0; input < numInputs; ++input)
    {
        feeders[input] = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
        feeders[input].call("feedBuffer", testValues.inputs[input]);
    }

    auto sink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;

        for (size_t input = 0; input < numInputs; ++input)
        {
            topology.connect(feeders[input], 0, arithmetic, input);
        }
        topology.connect(arithmetic, 0, sink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    auto output = sink.call<Pothos::BufferChunk>("getBuffer");
    POTHOS_TEST_EQUAL(output.dtype, testValues.expectedOutputs.dtype);
    POTHOS_TEST_EQUAL(output.elements(), testValues.expectedOutputs.elements());
    POTHOS_TEST_EQUALA(
        output.as<const T*>(),
        testValues.expectedOutputs.as<const T*>(),
        output.elements());
}

template <typename T>
static void testArithmetic()
{
    std::cout << "Testing " << Pothos::DType(typeid(T)).toString() << "..." << std::endl;

    testArithmeticOp<T>("ADD", getAddTestValues<T>());
    testArithmeticOp<T>("SUB", getSubTestValues<T>());
    testArithmeticOp<T>("MUL", getMulTestValues<T>());
    testArithmeticOp<T>("DIV", getDivTestValues<T>());
}

POTHOS_TEST_BLOCK("/comms/tests", test_arithmetic)
{
    testArithmetic<std::int8_t>();
    testArithmetic<std::int16_t>();
    testArithmetic<std::int32_t>();
    testArithmetic<std::int64_t>();
    testArithmetic<std::uint8_t>();
    testArithmetic<std::uint16_t>();
    testArithmetic<std::uint32_t>();
    testArithmetic<std::uint64_t>();
    testArithmetic<float>();
    testArithmetic<double>();
    testArithmetic<std::complex<std::int8_t>>();
    testArithmetic<std::complex<std::int16_t>>();
    testArithmetic<std::complex<std::int32_t>>();
    testArithmetic<std::complex<std::int64_t>>();
    testArithmetic<std::complex<std::uint8_t>>();
    testArithmetic<std::complex<std::uint16_t>>();
    testArithmetic<std::complex<std::uint32_t>>();
    testArithmetic<std::complex<std::uint64_t>>();
    testArithmetic<std::complex<float>>();
    testArithmetic<std::complex<double>>();
}

POTHOS_TEST_BLOCK("/comms/tests", test_arithmetic_feedback)
{
    auto registry = Pothos::ProxyEnvironment::make("managed")->findProxy("Pothos/BlockRegistry");

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto adder = Pothos::BlockRegistry::make("/comms/arithmetic", "int", "ADD");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");

    //adder has a preload on input1 for feedback loop
    std::vector<size_t> preload(2, 0); preload[1] = 1;
    adder.call("setPreload", preload);

    //load feeder block
    auto b0 = Pothos::BufferChunk(10 * sizeof(int));
    auto p0 = b0.as<int*>();
    for (int i = 0; i < 10; i++) p0[i] = i;
    feeder.call("feedBuffer", b0);

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, adder, 0);
        topology.connect(adder, 0, adder, 1);
        topology.connect(adder, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    //check the collector
    Pothos::BufferChunk buff = collector.call("getBuffer");
    POTHOS_TEST_EQUAL(buff.length, 10 * sizeof(int));
    auto pb = buff.as<const int*>();
    //for (int i = 0; i < 10; i++) std::cout << i << " " << pb[i] << std::endl;
    int last = 0;
    for (int i = 0; i < 10; i++)
    {
        last = i + last;
        POTHOS_TEST_EQUAL(pb[i], last);
    }
}

POTHOS_TEST_BLOCK("/comms/tests", test_inline_buffer)
{
    auto feeder0 = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto feeder1 = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto copier = Pothos::BlockRegistry::make("/blocks/copier");
    auto adder = Pothos::BlockRegistry::make("/comms/arithmetic", "int", "ADD");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");

    //load feeder blocks
    const auto numElems = 4000;
    auto b0 = Pothos::BufferChunk(numElems * sizeof(int));
    auto p0 = b0.as<int*>();
    for (int i = 0; i < numElems; i++) p0[i] = i;
    feeder0.call("feedBuffer", b0);

    auto b1 = Pothos::BufferChunk(numElems * sizeof(int));
    auto p1 = b1.as<int*>();
    for (int i = 0; i < numElems; i++) p1[i] = i + numElems;
    //feeder1.call("feedBuffer", b1);

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder0, 0, copier, 0); //copier before adder ensures framework provided buffers
        topology.connect(copier, 0, adder, 0);
        topology.connect(feeder1, 0, adder, 1);
        topology.connect(adder, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
        feeder1.call("feedBuffer", b1); //ensure that the buffer will be inlined by forcing processing on a non port 0 message
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    //check the collector
    Pothos::BufferChunk buff = collector.call("getBuffer");
    POTHOS_TEST_EQUAL(buff.length, numElems * sizeof(int));
    auto pb = buff.as<const int*>();
    //for (int i = 0; i < numElems; i++) std::cout << i << " " << pb[i] << std::endl;
    for (int i = 0; i < numElems; i++) POTHOS_TEST_EQUAL(pb[i], i + i + numElems);

    size_t numInlines = adder.call("getNumInlineBuffers");
    std::cout << "NumInlineBuffers " << numInlines << std::endl;
    POTHOS_TEST_TRUE(numInlines > 0);
}

//
// /comms/const_arithmetic
//

struct ConstArithmeticTestValues
{
    Pothos::BufferChunk inputs;
    Pothos::Object constant;
    Pothos::BufferChunk expectedOutputs;

    template <typename T>
    void setup(const T& testConstant, size_t bufferLength)
    {
        static const Pothos::DType dtype(typeid(T));

        inputs = Pothos::BufferChunk(typeid(T), bufferLength);
        constant = Pothos::Object(testConstant);
        expectedOutputs = Pothos::BufferChunk(dtype, bufferLength);
    }
};

template <typename T>
static inline typename std::enable_if<!IsComplex<T>::value, T>::type getXByKTestConstant()
{
    return T(2);
}

template <typename T>
static inline typename std::enable_if<IsComplex<T>::value, T>::type getXByKTestConstant()
{
    return T(3, 2);
}

template <typename T>
static void getXByKTestValues(
    ConstArithmeticTestValues* pXPlusKTestValues,
    ConstArithmeticTestValues* pXMinusKTestValues,
    ConstArithmeticTestValues* pXMulKTestValues,
    ConstArithmeticTestValues* pXDivKTestValues)
{
    const auto testConstant = getXByKTestConstant<T>();

    pXPlusKTestValues->setup(testConstant, bufferLen);
    pXMinusKTestValues->setup(testConstant, bufferLen);
    pXMulKTestValues->setup(testConstant, bufferLen);
    pXDivKTestValues->setup(testConstant, bufferLen);

    // Avoid setting input to 0 when unsigned
    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        auto value = static_cast<T>(elem + 2);
        if (std::is_signed<T>::value) value -= static_cast<T>(bufferLen / 2);

        pXPlusKTestValues->inputs.as<T*>()[elem] = value;
        pXMinusKTestValues->inputs.as<T*>()[elem] = value;
        pXMulKTestValues->inputs.as<T*>()[elem] = value;
        pXDivKTestValues->inputs.as<T*>()[elem] = value;
        
        pXPlusKTestValues->expectedOutputs.as<T*>()[elem] = value + testConstant;
        pXMinusKTestValues->expectedOutputs.as<T*>()[elem] = value - testConstant;
        pXMulKTestValues->expectedOutputs.as<T*>()[elem] = value * testConstant;
        pXDivKTestValues->expectedOutputs.as<T*>()[elem] = value / testConstant;
    }
}

template <typename T>
static inline typename std::enable_if<!IsComplex<T>::value, T>::type getKByXTestConstant(size_t bufferLength)
{
    return T(bufferLength + 2);
}

template <typename T>
static inline typename std::enable_if<IsComplex<T>::value, T>::type getKByXTestConstant(size_t bufferLength)
{
    using ScalarType = typename T::value_type;

    return T(ScalarType(bufferLength+2), ScalarType(bufferLength+1));
}

template <typename T>
static void getKByXTestValues(
    ConstArithmeticTestValues* pKMinusXTestValues,
    ConstArithmeticTestValues* pKDivXTestValues)
{
    const auto testConstant = getKByXTestConstant<T>(bufferLen);

    pKMinusXTestValues->setup(testConstant, bufferLen);
    pKDivXTestValues->setup(testConstant, bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        auto value = static_cast<T>(elem + 1);
        if (std::is_signed<T>::value)
        {
            value -= static_cast<T>(bufferLen / 2);
            if (T(0) == value) value += T(1);
        }

        pKMinusXTestValues->inputs.as<T*>()[elem] = value;
        pKDivXTestValues->inputs.as<T*>()[elem] = value;

        pKMinusXTestValues->expectedOutputs.as<T*>()[elem] = testConstant - value;
        pKDivXTestValues->expectedOutputs.as<T*>()[elem] = testConstant / value;
    }
}

template <typename T>
static void testConstArithmeticOp(
    const std::string& operation,
    const ConstArithmeticTestValues& testValues)
{
    const Pothos::DType dtype(typeid(T));

    std::cout << " * Testing " << operation << "..." << std::endl;

    auto constArithmetic = Pothos::BlockRegistry::make(
                                "/comms/const_arithmetic",
                                dtype,
                                operation,
                                testValues.constant);
    POTHOS_TEST_EQUAL(testValues.constant.extract<T>(), constArithmetic.call<T>("constant"));

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    feeder.call("feedBuffer", testValues.inputs);

    auto sink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;

        topology.connect(feeder, 0, constArithmetic, 0);
        topology.connect(constArithmetic, 0, sink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    auto output = sink.call<Pothos::BufferChunk>("getBuffer");
    POTHOS_TEST_EQUAL(output.dtype, testValues.expectedOutputs.dtype);
    POTHOS_TEST_EQUAL(output.elements(), testValues.expectedOutputs.elements());
    POTHOS_TEST_EQUALA(
        output.as<const T*>(),
        testValues.expectedOutputs.as<const T*>(),
        output.elements());
}

template <typename T>
static void testConstArithmetic()
{
    std::cout << "Testing " << Pothos::DType(typeid(T)).toString() << "..." << std::endl;

    ConstArithmeticTestValues xPlusKTestValues;
    ConstArithmeticTestValues xMinusKTestValues;
    ConstArithmeticTestValues kMinusXTestValues;
    ConstArithmeticTestValues xMulKTestValues;
    ConstArithmeticTestValues xDivKTestValues;
    ConstArithmeticTestValues kDivXTestValues;

    getXByKTestValues<T>(
        &xPlusKTestValues,
        &xMinusKTestValues,
        &xMulKTestValues,
        &xDivKTestValues);
    getKByXTestValues<T>(
        &kMinusXTestValues,
        &kDivXTestValues);

    testConstArithmeticOp<T>("X+K", xPlusKTestValues);
    testConstArithmeticOp<T>("X-K", xMinusKTestValues);
    testConstArithmeticOp<T>("K-X", kMinusXTestValues);
    testConstArithmeticOp<T>("X*K", xMulKTestValues);
    testConstArithmeticOp<T>("X/K", xDivKTestValues);
    testConstArithmeticOp<T>("K/X", kDivXTestValues);
}

POTHOS_TEST_BLOCK("/comms/tests", test_const_arithmetic)
{
    testConstArithmetic<std::int8_t>();
    testConstArithmetic<std::int16_t>();
    testConstArithmetic<std::int32_t>();
    testConstArithmetic<std::int64_t>();
    testConstArithmetic<std::uint8_t>();
    testConstArithmetic<std::uint16_t>();
    testConstArithmetic<std::uint32_t>();
    testConstArithmetic<std::uint64_t>();
    testConstArithmetic<float>();
    testConstArithmetic<double>();
    testConstArithmetic<std::complex<std::int8_t>>();
    testConstArithmetic<std::complex<std::int16_t>>();
    testConstArithmetic<std::complex<std::int32_t>>();
    testConstArithmetic<std::complex<std::int64_t>>();
    testConstArithmetic<std::complex<std::uint8_t>>();
    testConstArithmetic<std::complex<std::uint16_t>>();
    testConstArithmetic<std::complex<std::uint32_t>>();
    testConstArithmetic<std::complex<std::uint64_t>>();
    testConstArithmetic<std::complex<float>>();
    testConstArithmetic<std::complex<double>>();
}