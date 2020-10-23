// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <Poco/Format.h>

#include <complex>
#include <cstring>

//
// Default implementations
//

#define BITWISE_UNARY_ARRAY_LAMBDA(T,op) \
    [](const T* in, T* out, size_t len) \
    { \
        for(size_t elem = 0; elem < len; ++elem) out[elem] = op in[elem]; \
    }

#define BITWISE_BINARY_ARRAY_LAMBDA(T,op) \
    [](const T** in, T* out, size_t numInputs, size_t len) \
    { \
        std::memcpy(out, in[0], len * sizeof(T)); \
        for(size_t input = 1; input < numInputs; ++input) \
        { \
            for(size_t elem = 0; elem < len; ++elem) out[elem] = out[elem] op in[input][elem]; \
        } \
    }

#define BITWISE_BINARY_CONST_LAMBDA(T,op) \
    [](const T* in, T* out, T k, size_t len) \
    { \
        for(size_t elem = 0; elem < len; ++elem) out[elem] = in[elem] op k; \
    }

#define BITSHIFT_LAMBDA(T,op) \
    [](const T* in, T* out, size_t shiftSize, size_t len) \
    { \
        for(size_t elem = 0; elem < len; ++elem) out[elem] = in[elem] op shiftSize; \
    }

//
// Getter functions for implementations, to be called on class construction. This
// easily allows for alternative implementations.
//

template <typename T>
using BitwiseUnaryArrayFcn = void(*)(const T*, T*, size_t);

template <typename T>
using BitwiseBinaryArrayFcn = void(*)(const T**, T*, size_t, size_t);

template <typename T>
using BitwiseBinaryConstFcn = void(*)(const T*, T*, T, size_t);

template <typename T>
using BitShiftArrayFcn = void(*)(const T*, T*, size_t, size_t);

template <typename T>
static inline BitwiseUnaryArrayFcn<T> getNotFcn()
{
    return BITWISE_UNARY_ARRAY_LAMBDA(T, ~);
}

template <typename T>
static inline BitwiseBinaryArrayFcn<T> getAndArrayFcn()
{
    return BITWISE_BINARY_ARRAY_LAMBDA(T, &);
}

template <typename T>
static inline BitwiseBinaryArrayFcn<T> getOrArrayFcn()
{
    return BITWISE_BINARY_ARRAY_LAMBDA(T, |);
}

template <typename T>
static inline BitwiseBinaryArrayFcn<T> getXOrArrayFcn()
{
    return BITWISE_BINARY_ARRAY_LAMBDA(T, ^);
}

template <typename T>
static inline BitwiseBinaryConstFcn<T> getAndConstFcn()
{
    return BITWISE_BINARY_CONST_LAMBDA(T, &);
}

template <typename T>
static inline BitwiseBinaryConstFcn<T> getOrConstFcn()
{
    return BITWISE_BINARY_CONST_LAMBDA(T, |);
}

template <typename T>
static inline BitwiseBinaryConstFcn<T> getXOrConstFcn()
{
    return BITWISE_BINARY_CONST_LAMBDA(T, ^);
}

template <typename T>
static inline BitShiftArrayFcn<T> getLeftShiftFcn()
{
    return BITSHIFT_LAMBDA(T, <<);
}

template <typename T>
static inline BitShiftArrayFcn<T> getRightShiftFcn()
{
    return BITSHIFT_LAMBDA(T, >>);
}

//
// Block class implementation
//

template <typename Type>
class BitwiseUnaryArray : public Pothos::Block
{
public:
    BitwiseUnaryArray(size_t dimension, BitwiseUnaryArrayFcn<Type> fcn): _fcn(fcn)
    {
        const Pothos::DType dtype(typeid(Type), dimension);

        this->setupInput(0, dtype);
        this->setupOutput(0, dtype);
    }

    virtual ~BitwiseUnaryArray() = default;

    void work() override
    {
        const auto elems = this->workInfo().minElements;
        if (0 == elems) return;

        auto* inPort = this->input(0);
        auto* outPort = this->output(0);

        const auto N = elems * outPort->dtype().dimension();

        _fcn(inPort->buffer(), outPort->buffer(), N);

        inPort->consume(elems);
        outPort->produce(elems);
    }

private:
    BitwiseUnaryArrayFcn<Type> _fcn;
};

template <typename Type>
class BitwiseBinaryArray : public Pothos::Block
{
public:
    BitwiseBinaryArray(
        size_t dimension,
        size_t nchans,
        BitwiseBinaryArrayFcn<Type> fcn): _fcn(fcn), _nchans(nchans)
    {
        const Pothos::DType dtype(typeid(Type), dimension);

        for (size_t chan = 0; chan < _nchans; ++chan) this->setupInput(chan, dtype);
        this->setupOutput(0, dtype);
    }

    virtual ~BitwiseBinaryArray() = default;

    void work() override
    {
        const auto& workInfo = this->workInfo();

        const auto elems = workInfo.minElements;
        if (0 == elems) return;

        auto inPorts = this->inputs();
        auto outPort = this->output(0);
        auto inPtrs = workInfo.inputPointers.data();

        const auto N = elems * outPort->dtype().dimension();

        _fcn((const Type**)inPtrs, outPort->buffer(), _nchans, N);

        for (auto* inPort : inPorts) inPort->consume(elems);
        outPort->produce(elems);
    }

private:
    BitwiseBinaryArrayFcn<Type> _fcn;
    size_t _nchans;
};

template <typename Type>
class BitwiseBinaryConst : public Pothos::Block
{
public:
    BitwiseBinaryConst(
        size_t dimension,
        const Type& constant,
        BitwiseBinaryConstFcn<Type> fcn) : _fcn(fcn)
    {
        const Pothos::DType dtype(typeid(Type), dimension);

        this->setupInput(0, dtype);
        this->setupOutput(0, dtype);

        using Class = BitwiseBinaryConst<Type>;

        this->registerCall(this, POTHOS_FCN_TUPLE(Class, constant));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setConstant));

        this->registerProbe("constant");
        this->registerSignal("constantChanged");

        // Set here to emit signal
        this->setConstant(constant);
    }

    virtual ~BitwiseBinaryConst() = default;

    Type constant() const
    {
        return _constant;
    }

    void setConstant(const Type& constant)
    {
        _constant = constant;
        this->emitSignal("constantChanged", constant);
    }

    void work() override
    {
        const auto elems = this->workInfo().minElements;
        if (0 == elems) return;

        auto* inPort = this->input(0);
        auto* outPort = this->output(0);

        const auto N = elems * outPort->dtype().dimension();

        _fcn(inPort->buffer(), outPort->buffer(), _constant, N);

        inPort->consume(elems);
        outPort->produce(elems);
    }

private:
    BitwiseBinaryConstFcn<Type> _fcn;
    Type _constant;
};

template <typename Type>
class BitShift: public Pothos::Block
{
public:
    BitShift(size_t dimension, bool leftShift, size_t shiftSize):
        _leftShift(leftShift),
        _fcn(leftShift ? getLeftShiftFcn<Type>() : getRightShiftFcn<Type>())
    {
        const Pothos::DType dtype(typeid(Type), dimension);

        this->setupInput(0, dtype);
        this->setupOutput(0, dtype);

        using Class = BitShift<Type>;

        this->registerCall(this, POTHOS_FCN_TUPLE(Class, shiftSize));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setShiftSize));

        this->registerProbe("shiftSize");
        this->registerSignal("shiftSizeChanged");

        // Use caller to validate input and emit signal.
        this->setShiftSize(shiftSize);
    }

    virtual ~BitShift() = default;

    size_t shiftSize() const
    {
        return _shiftSize;
    }

    void setShiftSize(size_t shiftSize)
    {
        if (shiftSize >= (sizeof(Type)*8))
        {
            throw Pothos::RangeException(
                      Poco::format(
                          "Shift size cannot be >= the number of bits (%z) in the type (%s)",
                          (sizeof(Type)*8),
                          Pothos::DType(typeid(Type)).toString()));
        }

        _shiftSize = shiftSize;
        this->emitSignal("shiftSizeChanged", _shiftSize);
    }

    void work() override
    {
        const auto elems = this->workInfo().minElements;
        if (0 == elems) return;

        auto* inPort = this->input(0);
        auto* outPort = this->output(0);

        const auto N = elems * outPort->dtype().dimension();

        _fcn(inPort->buffer(), outPort->buffer(), _shiftSize, N);

        inPort->consume(elems);
        outPort->produce(elems);
    }

private:
    bool _leftShift;
    size_t _shiftSize;
    BitShiftArrayFcn<Type> _fcn;
};

//
// Factories
//

class BitwiseParamException: public Pothos::InvalidArgumentException
{
public:
    BitwiseParamException(
        const Pothos::DType& dtype,
        const std::string& operation
    ):
    Pothos::InvalidArgumentException(Poco::format("DType: %s, Operation: %s", dtype.toString(), operation))
    {}
};

static Pothos::Block* makeBitwiseUnaryArray(
    const Pothos::DType& dtype,
    const std::string& operation)
{
    #define BitwiseUnaryArrayFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T)) && (operation == "NOT")) \
                return new BitwiseUnaryArray<T>(dtype.dimension(), getNotFcn<T>());

    BitwiseUnaryArrayFactory(std::int8_t)
    else BitwiseUnaryArrayFactory(std::int16_t)
    else BitwiseUnaryArrayFactory(std::int32_t)
    else BitwiseUnaryArrayFactory(std::int64_t)
    else BitwiseUnaryArrayFactory(std::uint8_t)
    else BitwiseUnaryArrayFactory(std::uint16_t)
    else BitwiseUnaryArrayFactory(std::uint32_t)
    else BitwiseUnaryArrayFactory(std::uint64_t)

    throw BitwiseParamException(dtype, operation);
}

static Pothos::Block* makeBitwiseBinaryArray(
    const Pothos::DType& dtype,
    const std::string& operation,
    size_t numChannels)
{
    #define _BitwiseBinaryArrayFactory(T,op,fcn) \
        if(operation == op) return new BitwiseBinaryArray<T>(dtype.dimension(), numChannels, fcn<T>());

    #define BitwiseBinaryArrayFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
        { \
            _BitwiseBinaryArrayFactory     (T, "AND", getAndArrayFcn) \
            else _BitwiseBinaryArrayFactory(T, "OR",  getOrArrayFcn) \
            else _BitwiseBinaryArrayFactory(T, "XOR", getXOrArrayFcn) \
        }

    BitwiseBinaryArrayFactory(std::int8_t)
    else BitwiseBinaryArrayFactory(std::int16_t)
    else BitwiseBinaryArrayFactory(std::int32_t)
    else BitwiseBinaryArrayFactory(std::int64_t)
    else BitwiseBinaryArrayFactory(std::uint8_t)
    else BitwiseBinaryArrayFactory(std::uint16_t)
    else BitwiseBinaryArrayFactory(std::uint32_t)
    else BitwiseBinaryArrayFactory(std::uint64_t)

    throw BitwiseParamException(dtype, operation);

    // To stop compiler complaining, since it doesn't know the above function throws
    return nullptr;
}

static Pothos::Block* makeBitwiseBinaryConst(
    const Pothos::DType& dtype,
    const Pothos::Object& constant,
    const std::string& operation)
{
    #define _BitwiseBinaryConstFactory(T,op,fcn) \
        if(operation == op) return new BitwiseBinaryConst<T>(dtype.dimension(), constant.convert<T>(), fcn<T>());

    #define BitwiseBinaryConstFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
        { \
            _BitwiseBinaryConstFactory     (T, "AND", getAndConstFcn) \
            else _BitwiseBinaryConstFactory(T, "OR",  getOrConstFcn) \
            else _BitwiseBinaryConstFactory(T, "XOR", getXOrConstFcn) \
        }

    BitwiseBinaryConstFactory(std::int8_t)
    else BitwiseBinaryConstFactory(std::int16_t)
    else BitwiseBinaryConstFactory(std::int32_t)
    else BitwiseBinaryConstFactory(std::int64_t)
    else BitwiseBinaryConstFactory(std::uint8_t)
    else BitwiseBinaryConstFactory(std::uint16_t)
    else BitwiseBinaryConstFactory(std::uint32_t)
    else BitwiseBinaryConstFactory(std::uint64_t)

    throw BitwiseParamException(dtype, operation);
}

static Pothos::Block* makeBitShift(
    const Pothos::DType& dtype,
    const std::string& operation,
    size_t shiftSize)
{
    #define BitShiftFactory(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
        { \
            if(operation == "LEFTSHIFT")       return new BitShift<T>(dtype.dimension(), true, shiftSize); \
            else if(operation == "RIGHTSHIFT") return new BitShift<T>(dtype.dimension(), false, shiftSize); \
        }
    
    BitShiftFactory(std::int8_t)
    else BitShiftFactory(std::int16_t)
    else BitShiftFactory(std::int32_t)
    else BitShiftFactory(std::int64_t)
    else BitShiftFactory(std::uint8_t)
    else BitShiftFactory(std::uint16_t)
    else BitShiftFactory(std::uint32_t)
    else BitShiftFactory(std::uint64_t)

    throw BitwiseParamException(dtype, operation);
}

//
// Registration
//

/***********************************************************************
 * |PothosDoc Bitwise Unary Operation
 *
 * Perform a bitwise unary operation on an input buffer.
 *
 * |category /Digital
 * |keywords not
 *
 * |param dtype[Data Type] The block data type.
 * |widget DTypeChooser(int=1,uint=1,dim=1)
 * |default "uint64"
 * |preview disable
 *
 * |param operation The bitwise operation to perform.
 * |default "NOT"
 * |option [Not] "NOT"
 * |preview enable
 *
 * |factory /comms/bitwise_unary(dtype,operation)
 **********************************************************************/
static Pothos::BlockRegistry registerBitwiseUnaryArray(
    "/comms/bitwise_unary",
    Pothos::Callable(&makeBitwiseUnaryArray));

/***********************************************************************
 * |PothosDoc Bitwise Binary Operation
 *
 * Perform a bitwise binary operation on a set of input ports,
 * outputting the result to a single output buffer.
 *
 * |category /Digital
 * |keywords and not xor
 *
 * |param dtype[Data Type] The block data type.
 * |widget DTypeChooser(int=1,uint=1,dim=1)
 * |default "uint64"
 * |preview disable
 *
 * |param operation The bitwise operation to perform.
 * |default "AND"
 * |option [And] "AND"
 * |option [Or] "OR"
 * |option [XOr] "XOR"
 * |preview enable
 *
 * |param numChannels[Num Channels] The number of input ports.
 * |widget SpinBox(minimum=2)
 * |default 2
 * |preview disable
 *
 * |factory /comms/bitwise_binary(dtype,operation,numChannels)
 **********************************************************************/
static Pothos::BlockRegistry registerBitwiseBinaryArray(
    "/comms/bitwise_binary",
    Pothos::Callable(&makeBitwiseBinaryArray));

/***********************************************************************
 * |PothosDoc Bitwise Binary Const Operation
 *
 * Perform a bitwise binary operation on an input buffer and a specified
 * constant.
 *
 * |category /Digital
 * |keywords and not xor
 *
 * |param dtype[Data Type] The block data type.
 * |widget DTypeChooser(int=1,uint=1,dim=1)
 * |default "uint64"
 * |preview disable
 *
 * |param constant[Constant] The scalar value input for the bitwise operation.
 * |widget SpinBox()
 * |default 0
 * |preview enable
 *
 * |param operation The bitwise operation to perform.
 * |default "AND"
 * |option [And] "AND"
 * |option [Or] "OR"
 * |option [XOr] "XOR"
 * |preview enable
 *
 * |factory /comms/const_bitwise_binary(dtype,constant,operation)
 * |setter setConstant(constant)
 **********************************************************************/
static Pothos::BlockRegistry registerBitwiseBinaryConst(
    "/comms/const_bitwise_binary",
    Pothos::Callable(&makeBitwiseBinaryConst));

/***********************************************************************
 * |PothosDoc Bit Shift
 *
 * Perform a bitwise operation on the given input buffer.
 *
 * |category /Digital
 * |keywords left right
 *
 * |param dtype[Data Type] The block data type.
 * |widget DTypeChooser(int=1,uint=1,dim=1)
 * |default "uint64"
 * |preview disable
 *
 * |param operation The bit shift operation to perform.
 * |default "LEFTSHIFT"
 * |option [Left Shift] "LEFTSHIFT"
 * |option [Right Shift] "RIGHTSHIFT"
 * |preview enable
 *
 * |param shiftSize[Shift Size] The number of bits to shift.
 * |widget SpinBox(minimum=0)
 * |default 0
 * |preview enable
 *
 * |factory /comms/bitshift(dtype,operation,shiftSize)
 * |setter setShiftSize(shiftSize)
 **********************************************************************/
static Pothos::BlockRegistry registerBitShift(
    "/comms/bitshift",
    Pothos::Callable(&makeBitShift));
